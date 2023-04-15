# ESP32 PS/2 mouse/keyboard emulation library
This library allows ESP32 to emulate ps2 keyboard and/or mouse.
PC fixes have been added from previous forkes to make it more reliable during post.

# Electrical connections

The following is an easy way to get started:

 * Get a male-to-male PS/2 cable.
 * Cut the cable.
 * Use a multimeter to find the pinout.
 * Solder the cable to an ESP32 as follows.

| ESP32 pin | PS/2 pin | example color |
| --------- | -------- | ------------- |
| (any)     | DATA     | brown         |
| (any)     | CLK      | orange        |
| GND       | GND      | green         |

There is no need to connect the VCC.

Note: ESP32 is **unofficially** 5V tolerant, so you can directly connect PS/2 pin to the board. However, it is ideal to use a logic level converter like as:

 * https://www.adafruit.com/product/757
 * https://www.ebay.com/itm/143385426765

# Usage

You can see examples in `examples` directory.

```cpp
#include <Arduino.h>
#include <esp32-ps2dev.h>

esp32_ps2dev::PS2Mouse mouse(17, 16);  // clk, data
esp32_ps2dev::PS2Keyboard keyboard(19, 18);  // clk, data

void setup() {
  mouse.begin();
  keyboard.begin();
}

void loop() {
  // move mouse
  // void move(int16_t x, int16_t y, int8_t wheel);
  // x: -255 to 255
  // y: -255 to 255
  // wheel: -7 to 8
  mouse.move(100, 100, 1);

  // press a button
  // void press(Button button);
  // button: esp32_ps2dev::PS2Mouse::Button::{LEFT,RIGHT,MIDDLE,BUTTON_4,BUTTON_5}
  mouse.press(esp32_ps2dev::PS2Mouse::Button::LEFT);

  // release a button
  // void release(Button button);
  // button: esp32_ps2dev::PS2Mouse::Button::{LEFT,RIGHT,MIDDLE,BUTTON_4,BUTTON_5}
  mouse.release(esp32_ps2dev::PS2Mouse::Button::LEFT);

  // click a button (press and release a button)
  // void click(Button button);
  // button: esp32_ps2dev::PS2Mouse::Button::{LEFT,RIGHT,MIDDLE,BUTTON_4,BUTTON_5}
  mouse.click(esp32_ps2dev::PS2Mouse::Button::LEFT);


  // press a key
  // void keydown(scancodes::Key key);
  // key: esp32_ps2dev::scancodes::Key::K_*
  keyboard.keydown(esp32_ps2dev::scancodes::Key::K_RETURN);

  // release a key
  // void keyup(scancodes::Key key);
  // key: esp32_ps2dev::scancodes::Key::K_*
  keyboard.keyup(esp32_ps2dev::scancodes::Key::K_RETURN);

  // press and release a key
  // void type(scancodes::Key key);
  // key: esp32_ps2dev::scancodes::Key::K_*
  keyboard.type(esp32_ps2dev::scancodes::Key::K_RETURN);

  // press and release keys
  // void type(std::initializer_list<scancodes::Key> keys);
  // keys: {esp32_ps2dev::scancodes::Key::K_*...}
  keyboard.type({esp32_ps2dev::scancodes::Key::K_LCTRL, esp32_ps2dev::scancodes::Key::K_A});

  // type string
  // void type(const char* str);
  // str: ASCII string
  keyboard.type("Hello, world!");
}

```

# TODO
 * Write more examples.
 * Improve stability.
 * Clean up the code.
 * Test against various hosts.

# Reference
- http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Protocol.htm
- http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Mouse%20Protocol.htm
- http://www-ug.eecg.toronto.edu/msl/nios_devices/datasheets/PS2%20Keyboard%20Protocol.htm

# History
Originaly from:
 * https://playground.arduino.cc/ComponentLib/Ps2mouse/
 * https://github.com/grappendorf/arduino-framework/tree/master/ps2dev
 * https://github.com/dpavlin/Arduino-projects/tree/master/libraries/ps2dev
 * Written by Chris J. Kiick, January 2008. https://github.com/ckiick
 * modified by Gene E. Scogin, August 2008.
 * modified by Tomas 'Harvie' Mudrunka 2019. https://github.com/harvie/ps2dev
 * modified for ESP32 by hrko 2022. https://github.com/hrko/esp32-ps2dev
