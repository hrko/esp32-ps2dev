#ifndef C05CFFFE_E405_4DD0_A541_EC07FFA90E99
#define C05CFFFE_E405_4DD0_A541_EC07FFA90E99

#include <initializer_list>
#include <stack>

#include "Arduino.h"
#include "Log.hpp"

namespace esp32_ps2dev {

// Time per clock should be 60 to 100 microseconds according to PS/2 specifications.
// Thus, half period should be 30 to 50 microseconds. 
const uint32_t CLK_HALF_PERIOD_MICROS = 30;
const uint32_t CLK_QUATER_PERIOD_MICROS = CLK_HALF_PERIOD_MICROS / 2;
// I could not find any specification of time between bytes from the PS/2 specification.
// Based on observation of the mouse signal waveform using an oscilloscope, there appears to be an interval of 1 to 2 clock cycles.
// ref. https://youtu.be/UqRDLWGLCEk
const uint32_t BYTE_INTERVAL_MICROS = 100;
// The device should check for "HOST_REQUEST_TO_SEND" at a interval not exceeding 10 milliseconds.
const uint32_t INTERVAL_CHECKING_HOST_SEND_REQUEST_MILLIS = 9;

const int PACKET_QUEUE_LENGTH = 20;
const UBaseType_t DEFAULT_TASK_PRIORITY = 10;
const BaseType_t DEFAULT_TASK_CORE = APP_CPU_NUM;
const uint32_t MOUSE_CLICK_PRESSING_DURATION_MILLIS = 100;

class PS2Packet {
 public:
  uint8_t len;
  uint8_t data[16];
};

class PS2dev {
 public:
  PS2dev(int clk, int data);

  enum class BusState {
    IDLE,
    COMMUNICATION_INHIBITED,
    HOST_REQUEST_TO_SEND,
  };

  void config(UBaseType_t task_priority, BaseType_t task_core);
  void begin();
  int write(unsigned char data);
  int read(unsigned char* data, uint64_t timeout_ms = 0);
  virtual int reply_to_host(uint8_t host_cmd) = 0;
  BusState get_bus_state();
  SemaphoreHandle_t get_bus_mutex_handle();
  QueueHandle_t get_packet_queue_handle();
  int send_packet_to_queue(const PS2Packet& packet);

 protected:
  int _ps2clk;
  int _ps2data;
  UBaseType_t _config_task_priority = DEFAULT_TASK_PRIORITY;
  BaseType_t _config_task_core = DEFAULT_TASK_CORE;
  TaskHandle_t _task_process_host_request;
  TaskHandle_t _task_send_packet;
  QueueHandle_t _queue_packet;
  SemaphoreHandle_t _mutex_bus;
  void golo(int pin);
  void gohi(int pin);
  void ack();
};

void _taskfn_process_host_request(void* arg);
void _taskfn_send_packet(void* arg);

}  // namespace esp32_ps2dev

#endif /* C05CFFFE_E405_4DD0_A541_EC07FFA90E99 */
