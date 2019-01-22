Spartronics countdown clock to the bag date. Scrolling message is supported which is set via the mode button

## Parts
- Teensy LC
- Adafruit NeoMatrix
- DS3231 RTC
- AT24C32 for storage -- currently not used

## Libraries
- [IntervalTimer library](https://www.pjrc.com/teensy/td_timing_IntervalTimer.html)
- [DebounceEvent button manager](https://github.com/xoseperez/debounceevent)
- Adafruit NeoPixel and NeoMatrix library
- [DS1307RTC library (compatible w/ DS3231)](https://www.pjrc.com/teensy/td_libs_DS1307RTC.html)

## Usage
- 3 buttons control the behavior of the display
    - button 1: specifies mode for reprogramming the display, i.e. display clock or scrolling message. Mode button circulates: display message, display countdown clock, manually set the countdown days, hours, mins, secs
    - button 2 & 3: indicates increment/decrement values based on the mode

Note: holding the DEC button down during boot will force set the datetime via user input