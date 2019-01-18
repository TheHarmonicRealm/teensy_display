/*

  Debounce buttons and trigger events
  Copyright (C) 2015-2018 by Xose Pérez <xose dot perez at gmail dot com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Arduino.h>
#include <DebounceEvent.h>

#define MODE_BUTTON_PIN 14  // D14 (D == digital port) -- sets display mode
#define INC_BUTTON_PIN  15  // D15 -- increments value
#define DEC_BUTTON_PIN  16  // D16 -- decrements value

// forward declerations
const char *print_event(uint8_t e);
const char *print_pin(uint8_t p);

/**
 * Important: after sketch starts, set port to Teensy Serial
 * Tools/Ports --> Teensy LC Serial
 */
void callback(uint8_t pin, uint8_t event, uint8_t count, uint16_t length) {
    Serial.print("PIN : "); Serial.print(print_pin(pin));
    Serial.print(" Event : "); Serial.print(print_event(event));
    Serial.print(" Count : "); Serial.print(count);
    Serial.print(" Length: "); Serial.print(length);
    Serial.println();
    delay(1000);
}

const char *print_event(uint8_t e)
{
    switch (e)
    {
        case EVENT_NONE:
            return ("None:0");
        case EVENT_CHANGED:
            return ("Changed:1");
        case EVENT_PRESSED:
            return ("Pressed:2");
        case EVENT_RELEASED:
            return ("Released:3");
    }
    return ("WTF!?");
}

const char *print_pin(uint8_t p)
{
    switch (p)
    {
        case MODE_BUTTON_PIN:
            return ("mode:14");
        case INC_BUTTON_PIN:
            return ("inc:15");
        case DEC_BUTTON_PIN:
            return ("dec:16");
    }
    return ("WTF!?");
}

DebounceEvent mode_button = DebounceEvent(MODE_BUTTON_PIN, callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
DebounceEvent inc_button = DebounceEvent(INC_BUTTON_PIN, callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
DebounceEvent dec_button = DebounceEvent(DEC_BUTTON_PIN, callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);

void setup() {
    Serial.begin(9600);
    Serial.println();
    Serial.println();
}

void loop() {
    mode_button.loop();
    inc_button.loop();
    dec_button.loop();
}
