#include "PS2Dev.hpp"

namespace esp32_ps2dev {

PS2dev::PS2dev(int clk, int data) {
  _ps2clk = clk;
  _ps2data = data;
}

void PS2dev::config(UBaseType_t task_priority, BaseType_t task_core) {
  if (task_priority < 1) {
    task_priority = 1;
  } else if (task_priority > configMAX_PRIORITIES) {
    task_priority = configMAX_PRIORITIES - 1;
  }
  _config_task_priority = task_priority;
  _config_task_core = task_core;
}

void PS2dev::begin() {
  gohi(_ps2clk);
  gohi(_ps2data);
  _mutex_bus = xSemaphoreCreateMutex();
  _queue_packet = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(PS2Packet*));
  xTaskCreateUniversal(_taskfn_process_host_request, "process_host_request", 4096, this, _config_task_priority, &_task_process_host_request,
                       _config_task_core);
  xTaskCreateUniversal(_taskfn_send_packet, "send_packet", 4096, this, _config_task_priority - 1, &_task_send_packet, _config_task_core);
}

void PS2dev::gohi(int pin) {
  digitalWrite(pin, HIGH);
  pinMode(pin, INPUT);
}

void PS2dev::golo(int pin) {
  pinMode(pin, OUTPUT_OPEN_DRAIN);
  digitalWrite(pin, LOW);
}

void PS2dev::ack() {
  delayMicroseconds(BYTE_INTERVAL_MICROS);
  write(0xFA);
  delayMicroseconds(BYTE_INTERVAL_MICROS);
}

int PS2dev::write(unsigned char data) {
  unsigned char i;
  unsigned char parity = 1;

  if (get_bus_state() != BusState::IDLE) {
    return -1;
  }

  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);

  golo(_ps2data);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  // device sends on falling clock
  golo(_ps2clk);  // start bit
  delayMicroseconds(CLK_HALF_PERIOD_MICROS);
  gohi(_ps2clk);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

  for (i = 0; i < 8; i++) {
    if (data & 0x01) {
      gohi(_ps2data);
    } else {
      golo(_ps2data);
    }
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }
  // parity bit
  if (parity) {
    gohi(_ps2data);
  } else {
    golo(_ps2data);
  }
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  golo(_ps2clk);
  delayMicroseconds(CLK_HALF_PERIOD_MICROS);
  gohi(_ps2clk);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

  // stop bit
  gohi(_ps2data);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  golo(_ps2clk);
  delayMicroseconds(CLK_HALF_PERIOD_MICROS);
  gohi(_ps2clk);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

  taskEXIT_CRITICAL(&mux);

  return 0;
}

int PS2dev::read(unsigned char* value, uint64_t timeout_ms) {
  unsigned int data = 0x00;
  unsigned int bit = 0x01;

  unsigned char calculated_parity = 1;
  unsigned char received_parity = 0;

  // wait for data line to go low and clock line to go high (or timeout)
  unsigned long waiting_since = millis();
  while (get_bus_state() != BusState::HOST_REQUEST_TO_SEND) {
    if ((millis() - waiting_since) > timeout_ms) return -1;
    delay(1);
  }

  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);

  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  golo(_ps2clk);
  delayMicroseconds(CLK_HALF_PERIOD_MICROS);
  gohi(_ps2clk);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

  while (bit < 0x0100) {
    if (digitalRead(_ps2data) == HIGH) {
      data = data | bit;
      calculated_parity = calculated_parity ^ 1;
    } else {
      calculated_parity = calculated_parity ^ 0;
    }

    bit = bit << 1;

    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  }
  // we do the delay at the end of the loop, so at this point we have
  // already done the delay for the parity bit

  // parity bit
  if (digitalRead(_ps2data) == HIGH) {
    received_parity = 1;
  }

  // stop bit
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  golo(_ps2clk);
  delayMicroseconds(CLK_HALF_PERIOD_MICROS);
  gohi(_ps2clk);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  golo(_ps2data);
  golo(_ps2clk);
  delayMicroseconds(CLK_HALF_PERIOD_MICROS);
  gohi(_ps2clk);
  delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
  gohi(_ps2data);

  taskEXIT_CRITICAL(&mux);

  *value = data & 0x00FF;

  if (received_parity == calculated_parity) {
    return 0;
  } else {
    return -2;
  }
}

PS2dev::BusState PS2dev::get_bus_state() {
  if (digitalRead(_ps2clk) == LOW) {
    return BusState::COMMUNICATION_INHIBITED;
  } else if (digitalRead(_ps2data) == LOW) {
    return BusState::HOST_REQUEST_TO_SEND;
  } else {
    return BusState::IDLE;
  }
}

SemaphoreHandle_t PS2dev::get_bus_mutex_handle() { return _mutex_bus; }
QueueHandle_t PS2dev::get_packet_queue_handle() { return _queue_packet; }

int IRAM_ATTR PS2dev::send_packet_to_queue(const PS2Packet& packet) {
  auto packet_copy = new PS2Packet(packet);
  return (xQueueSend(_queue_packet, &packet_copy, 0) == pdTRUE) ? 0 : -1;
}

void _taskfn_process_host_request(void* arg) {
  PS2dev* ps2dev = (PS2dev*)arg;
  while (true) {
    xSemaphoreTake(ps2dev->get_bus_mutex_handle(), portMAX_DELAY);
    if (ps2dev->get_bus_state() == PS2dev::BusState::HOST_REQUEST_TO_SEND) {
      uint8_t host_cmd;
      if (ps2dev->read(&host_cmd) == 0) {
        ps2dev->reply_to_host(host_cmd);
      }
    }
    xSemaphoreGive(ps2dev->get_bus_mutex_handle());
    delay(INTERVAL_CHECKING_HOST_SEND_REQUEST_MILLIS);
  }
  vTaskDelete(NULL);
}

void _taskfn_send_packet(void* arg) {
  PS2dev* ps2dev = (PS2dev*)arg;
  while (true) {
    PS2Packet* packet;
    if (xQueueReceive(ps2dev->get_packet_queue_handle(), &packet, portMAX_DELAY) == pdTRUE) {
      xSemaphoreTake(ps2dev->get_bus_mutex_handle(), portMAX_DELAY);
      if (ps2dev->get_bus_state() != PS2dev::BusState::IDLE) {
        continue;
      }
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      for (int i = 0; i < packet->len; i++) {
        if (ps2dev->get_bus_state() != PS2dev::BusState::IDLE) {
          break;
        }
        ps2dev->write(packet->data[i]);
        delayMicroseconds(BYTE_INTERVAL_MICROS);
      }
      xSemaphoreGive(ps2dev->get_bus_mutex_handle());
    }
    delete packet;
  }
  vTaskDelete(NULL);
}

}  // namespace esp32_ps2dev