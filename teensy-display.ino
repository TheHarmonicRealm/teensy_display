/**
 * Spartronics sample code for Teensy LC based display via Adafruit NeoMatrix
 *
 * Parts/Libraries:
 *  - Teensy LC
 *  - Adafruit NeoMatrix
 *  - DS3231 RTC
 *  - AT24C32 for storage -- currently not used
 *
 * Libraries:
 *  - DebounceEvent button manager by Xose Perez https://github.com/xoseperez/debounceevent
 *  - IntervalTimer library for Teensy
 *  - DS1307RTC library (compatible w/ DS3231) https://www.pjrc.com/teensy/td_libs_DS1307RTC.html
 *
 * Usage:
 *  3 buttons control the behavior of the display
 *  - button 1: specifies mode for reprogramming the display: prints scrolling message or coundown clock
 *  - button 2 & 3: unused -- button push will generate corresponding events
 * Note: holding the DEC button down during boot will force set the datetime via user input
 *
 * Note: Spartronics clock also hosts an AT24C32 which is a 32K EEPROM. It is currently not used.
 * In the future, this can hold long messages or novels.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <DebounceEvent.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <stdbool.h>

#define DEBUG_ON    0

#define CHAR_HEIGHT 7

// 4x7 number digit font bitmaps -- used for displaying clockface
static const byte numbers[][CHAR_HEIGHT]
{
  // 0
  { B11110000,
    B10010000,
    B10010000,
    B10010000,
    B10010000,
    B10010000,
    B11110000 },
  //1
  { B00010000,
    B00010000,
    B00010000,
    B00010000,
    B00010000,
    B00010000,
    B00010000 },
  // 2
  { B11110000,
    B00010000,
    B00010000,
    B11110000,
    B10000000,
    B10000000,
    B11110000 },
  // 3
  { B11110000,
    B00010000,
    B00010000,
    B11110000,
    B00010000,
    B00010000,
    B11110000 },
  // 4
  { B10010000,
    B10010000,
    B10010000,
    B11110000,
    B00010000,
    B00010000,
    B00010000, },
  // 5
  {
    B11110000,
    B10000000,
    B10000000,
    B11110000,
    B00010000,
    B00010000,
    B11110000, },
  // 6
  { B11110000,
    B10000000,
    B10000000,
    B11110000,
    B10010000,
    B10010000,
    B11110000, },
  // 7
  { B11110000,
    B00010000,
    B00010000,
    B00010000,
    B00010000,
    B00010000,
    B00010000, },
  // 8
  { B11110000,
    B10010000,
    B10010000,
    B11110000,
    B10010000,
    B10010000,
    B11110000, },
  // 9
  { B11110000,
    B10010000,
    B10010000,
    B11110000,
    B00010000,
    B00010000,
    B11110000, },
};

static const byte colon_bitmap[] =
{
  // : (2 pix wide)
  B00000000,
  B10000000,
  B10000000,
  B00000000,
  B10000000,
  B10000000,
  B00000000,
};

/**
 * Setup the display state
 *      - countdown: displays the countdown clock -- updated by timer_ticks
 *      - days/hrs/mins/secs/: changes the value using inc/dec button presses
 *      - message: prints text messages
 */
typedef enum   // at each button mode press, display mode changes accordingly
{
    STATE_MESSAGE = 0,
    STATE_COUNTDOWN,
    // Add new states above this line
    STATE_MAX
} State_t;

/**
 * Setup the events for triggering state changes
 * These are driven by the buttons
 */
typedef enum
{
    EVENT_NULL = 0,     // no event
    EVENT_MODE,
    EVENT_INCREMENT,
    EVENT_DECREMENT,
    EVENT_TIMER,
    EVENT_SCROLL,
    // Add new events above this line
    EVENT_MAX
} Event_t;

State_t state;                                             // tracks the state of the display
const char *message = { "Spartronics: 4915. Woot!" }; // prints the message on display

uint32_t countdown_time;    // tracks the countdown time to bag date
typedef struct              // parses the countdown_time to its components
{
    uint8_t seconds, minutes, hours;
    uint16_t days;
} ElapsedTime_t;

/**
 * setup interrupts for scroll and timer events, callbacks, and variables to update controls
 */
IntervalTimer scroll_interval;
IntervalTimer timer_interval;

bool update_timer_event = false;
bool update_scroll_event = false;;

void timer_callback()
{
    update_timer_event = true;
}

void scroll_callback()
{
    update_scroll_event = true;
}

/** Good reference for display configuration:
 *  https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library
 *
 *  Spartronics display is setup as a 45x7 matrix.The first pixel is on top left,
 *  lines are arranged in rows as zig-zag order. The shield uses 800Khz (v2)
 *  pixels that expect GRB color data
 */
#define CONTROL_PIN 17      // pin for the NeoMatrix

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(45, 7,   // matrix width & height
  CONTROL_PIN,                                  // pin number
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +        // matrix layout, flags added as needed
  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);             // pixel type flags, added as needed


/**
 * For botton management using the DebounceEvent library
 * https://github.com/xoseperez/debounceevent
 */
#define MODE_BUTTON_PIN 14      // D14 (D == digital port) -- sets display mode
#define INC_BUTTON_PIN  15      // D15 -- increments value
#define DEC_BUTTON_PIN  16      // D16 -- decrements value

#define CUSTOM_DEBOUNCE_DELAY   50
#define CUSTOM_REPEAT_DELAY     0       // disabled double clicks

DebounceEvent *mode_button;
DebounceEvent *inc_button;
DebounceEvent *dec_button;

void event_callback(uint8_t pin, uint8_t event, uint8_t count, uint16_t length) {
    Event_t _event = EVENT_NULL;

    if (event != EVENT_PRESSED)
    {
        return;
    }

    switch (pin)
    {
        case MODE_BUTTON_PIN:
            _event = EVENT_MODE;
            break;
        case INC_BUTTON_PIN:
            _event = EVENT_INCREMENT;
            break;
        case DEC_BUTTON_PIN:
            _event = EVENT_DECREMENT;
            break;
        default:
            return;
    }
    // update the state machine
    state = state_machine(state, _event);
}

// colors used for the matrix display
const uint16_t colors[] = {
    matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255)};

/**
 * Set time: 1st get time from computer, 2nd write to RTC
 * Note: this code could be simplified by switching use of Serial.parseInt()
 */
time_t set_time()
{
    // prompt user for time and set it on the board
    state = STATE_MESSAGE;
    print_scroll("Enter time using Serial Port");

    // read & parse time from the user
    Serial.println("Make sure Serial Terminal is set to Newline");
    Serial.println("Enter today's date and time: YYYY:MM:DD:hh:mm:ss");

    // NOTE: sscanf can be replaced w/ Serial.parseInt()
    char *datetime = recvWithEndMarker();
    Serial.print("Received: "); Serial.println(datetime);
    int YYYY; int MM, DD, hh, mm, ss;
    if (sscanf(datetime, "%04d:%02d:%02d:%02d:%02d:%02d", &YYYY, &MM, &DD, &hh, &mm, &ss) != 6)
    {
        Serial.println("Error: problem parsing YYYY:MM:DD:hh:mm:ss");
        return 0;
    }

    // set RTC time
    time_t t = tmConvert_t (YYYY, (byte)MM, (byte)DD, (byte)hh, (byte)mm, (byte)ss);
    if (RTC.set(t) == false) // error occured while setting clock
    {
        state = STATE_MESSAGE;
        print_scroll("Error: RTC.set() failed!");
#if DEBUG_ON
        Serial.println("Error: RTC.set failed.");
#endif
    }

    return t;
}

// code from: http: //forum.arduino.cc/index.php?topic=396450
char *recvWithEndMarker()
{
    int numChars = 20;   // YYYY:MM:DD:hh:mm:ss
    static char receivedChars[20];
    boolean newData = false;
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    while (newData == false)
    {
        while (Serial.available() > 0)
        {
            rc = Serial.read();

            if (rc != endMarker)
            {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars)
                {
                    ndx = numChars - 1;
                }
            }
            else
            {
                receivedChars[ndx] = '\0'; // terminate the string
                ndx = 0;
                newData = true;
                break;
            }
        }
        delay(100);
    }
    return receivedChars;
}


/**
 * setup the display, matrix, and button callbacks
 **/
void setup()
{
    Serial.begin(115200);

    // setup matrix display
    matrix.begin();
    matrix.setTextWrap(false);
    matrix.setBrightness(40);
    matrix.setTextColor(colors[0]);

    // setup the button event loops
    mode_button = new DebounceEvent(MODE_BUTTON_PIN, event_callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
    inc_button = new DebounceEvent(INC_BUTTON_PIN, event_callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
    dec_button = new DebounceEvent(DEC_BUTTON_PIN, event_callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);

    // Check if the DEC_BUTTON_PIN is held low (pressed) while we boot up. If so, force time setting.
    bool force_set = (digitalRead(DEC_BUTTON_PIN) == LOW);
    Serial.print("force_set is: "); Serial.println(force_set);

    /**
     * Initialize the countdown_time using bag date and current date for countdown_time
     *      - via DS3231 RTC, read current time if available. If not, set current time using compiler time
     *      - bag date is set in the code
     * Use str format for creating date -- https://forum.arduino.cc/index.php?topic=465881.0
     *      - YYYY, MM, DD, HH, MM, SS
     */
    time_t now_date = 0;
    tmElements_t tm;

    if (force_set)
    {
        now_date = set_time();
    }

    if (!RTC.read(tm))    // no prior date is set
    {
        if (RTC.chipPresent())
        {
            if ((now_date = set_time()) == 0)   // error: failed to set RTC time!
            {
                state = STATE_MESSAGE;
                print_scroll("Error: RTC set time!");
#if DEBUG_ON
                Serial.println("RTC is stopped. Set time failed!");
#endif
            }
        }
        else
        {
            state = STATE_MESSAGE;
            print_scroll("Error: RTC circuit!");
#if DEBUG_ON
            Serial.println("RTC read error!  Please check the circuitry.");
#endif
        }
    }
    else
    {
        // Successfully got the time from the RTC!
        now_date = makeTime(tm);
    }

    if (now_date == 0)       // if now_date is null due to error, manually set the date
    {
        now_date = tmConvert_t(2019, 01, 5, 00, 00, 00);
    }
    time_t bag_date = tmConvert_t(2019, 02, 19, 20, 59, 00);
    countdown_time = bag_date - now_date;

#if DEBUG_ON
    Serial.print("bag_date: "); Serial.println(bag_date);
    Serial.print("now_date: "); Serial.println(now_date);
    Serial.print("countdown_time: "); Serial.println(countdown_time);
#endif

    // initialize state machine
    state = STATE_COUNTDOWN;

    // initialize the interval timer multiplied by 1000 to milliseconds
    timer_interval.begin(timer_callback, 1000*1000);
    scroll_interval.begin(scroll_callback, 100*1000);
}

/**
 * continously loop through button callbacks to detect display state changes
 * if no button presses, update the display accordingly
 *      - if in countdown mode, wait for timer_ticks to adjust seconds, etc.
 *      - if in sayings mode, print the 'sayings' on the matrix
 *      - in any other mode, wait for inc/dec button presses
 */
void loop()
{
    // listen for button callbacks and trigger state_machine change accordingly
    mode_button->loop();
    inc_button->loop();
    dec_button->loop();

    // listen for timer events & update state machine
    if (update_timer_event)
    {
        update_timer_event = false;
        state = state_machine(state, EVENT_TIMER);
    }
    else if (update_scroll_event)
    {
        update_scroll_event = false;
        state = state_machine(state, EVENT_SCROLL);
    }
}

/**
 * Time functions:
 *  - tmConvert_t: given a date info, makes time -- returns time in seconds since epoch
 *  - compute_elapsedTime: given a ulong, parses DD:HH:MM:SS -- see: https://pastebin.com/sfEjA94n
 */
time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
    tmElements_t tmSet;

    tmSet.Year = YYYY - 1970;
    tmSet.Month = MM;
    tmSet.Day = DD;
    tmSet.Hour = hh;
    tmSet.Minute = mm;
    tmSet.Second = ss;

    return makeTime(tmSet); //convert to time_t
}

void compute_elapsedTime(ElapsedTime_t *t)
{
    uint32_t difference = countdown_time;

    t->seconds = difference % 60;
    difference /= 60; // now it is minutes

    t->minutes = difference % 60;
    difference /= 60; // now it is hours

    t->hours = difference % 24;
    difference /= 24; // now it is days

    t->days = difference;
}

int _pixel_size = 6;    // standard pixel size for Adafruit_GFX is 6px per character
int _x = matrix.width();
int _pass = 0;

/**
 * Display a scrolling message
 */
void print_scroll(const char *str)
{
    matrix.fillScreen(0);
    matrix.setCursor(_x, 0);
    matrix.print(F(str));
    /**
     * setCursor() sets the pixel position from where to render the text
     * offset is calculated based on font width, screen width, and text length
     * when offset is reached, pass is complete & color is updated
    **/
    int offset = (strlen(str)*_pixel_size) + _x;
    if (--_x < -offset)
    {
        _x = matrix.width();
        if (++_pass >= 3)
            _pass = 0;
        matrix.setTextColor(colors[_pass]);
    }
    matrix.show();
}


/**
 * Responsible for updating the state of the system
 * See State_t for possible states
 */
State_t advance_state(State_t current_state)
{
    // we are using casts to ensure we can do enum math
    int next_state = (int)current_state + 1;
    if (next_state >= STATE_MAX)
    {
        next_state = STATE_MESSAGE;
    }

    return (State_t)next_state;
}

/**
 * State machine to control the display
 * Note: events triggered by the buttons and timer_ticks
 */
State_t state_machine(State_t current_state, Event_t event)
{
    State_t next_state = current_state;

    if (event == EVENT_TIMER)
    {
        countdown_time = countdown_time - 1;
    }

    if (event == EVENT_MODE)
    {
        next_state = advance_state(current_state);
    }
    else
    {
        switch (current_state)
        {
            case STATE_COUNTDOWN:   // Fall through
                ElapsedTime_t elapsed_time;

                compute_elapsedTime(&elapsed_time);
    #if DEBUG_ON
                Serial.print("---countdown_time: "); Serial.println(countdown_time);
                Serial.print(" DD: "); Serial.println(elapsed_time.days);
                Serial.print(" HH: "); Serial.println(elapsed_time.hours);
                Serial.print(" MM: "); Serial.println(elapsed_time.minutes);
                Serial.print(" SS: "); Serial.println(elapsed_time.seconds);
    #endif

                // display the time value
                print_countdown(elapsed_time.days, elapsed_time.hours, elapsed_time.minutes, elapsed_time.seconds);
                break;

            case STATE_MESSAGE:
                print_scroll(message);
                break;

            // unhandled case -- don't know how we got here, so just set the default
            default:
                next_state = STATE_MESSAGE;
                break;
        }
    }

    return next_state;
}

/**
 * Print clock face: DD:HH:MM:SS
 * Requires bitmap print to support the size of the display
 */
static int _cursor = 0;
void clear_screen(void)
{
  matrix.clear();
  //matrix.show();
  _cursor = 0;
}

void print_digit(unsigned i, uint16_t color)
{
  int width = 5; // Digit width

  if (i > 10)
  {
    // Invalid data, nothing to do...
    return;
  }

  matrix.fillRect(_cursor, 0, _cursor + width, CHAR_HEIGHT, 0);
  matrix.drawBitmap(_cursor, 0, numbers[i], width, CHAR_HEIGHT, color);
  //matrix.show();
  _cursor += width;
}

void print_colon(uint16_t color)
{
  int width = 2; // Colon width

  matrix.fillRect(_cursor, 0, _cursor + width, CHAR_HEIGHT, 0);
  matrix.drawBitmap(_cursor, 0, colon_bitmap, width, CHAR_HEIGHT, color);
  //matrix.show();
  _cursor += width;
}

// Print a 2-digit number
void print_num(unsigned i, uint16_t color)
{
  uint8_t temp_digit;

  temp_digit = i % 10;
  i = i / 10;
  print_digit(i % 10, color);
  print_digit(temp_digit, color);
}

void print_countdown(unsigned days, unsigned hours, unsigned mins, unsigned secs)
{
  clear_screen();
  print_num(days, colors[1]);
  print_colon(colors[0]);
  print_num(hours, colors[1]);
  print_colon(colors[0]);
  print_num(mins, colors[1]);
  print_colon(colors[0]);
  print_num(secs, colors[1]);
  matrix.show();
}
