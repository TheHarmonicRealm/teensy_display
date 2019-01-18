/**
 * Spartronics sample code for Teensy LC based display via Adafruit NeoMatrix
 *
 * Parts/Libraries:
 *  - Teensy LC
 *  - Adafruit NeoMatrix
 *  - DebounceEvent button manager by Xose Perez
 *  - everytime library by Karl Fessel
 *
 * Usage:
 *  3 buttons control the behavior of the display
 *  - button 1: specifies mode for reprogramming the display
 *  - button 2 & 3: indicates increment/decrement values based on the mode
 */

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <DebounceEvent.h>
#include <TimeLib.h>
//#include <stdbool.h>
#include <everytime.h>

#define DEBUG_ON    1

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
    STATE_DAYS,
    STATE_HOURS,
    STATE_MINUTES,
    STATE_SECONDS,
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
    // Add new events above this line
    EVENT_MAX
} Event_t;

State_t state;                                             // tracks the state of the display
const char *message = { "Welcome to Spartronics Open House!" }; // prints the message on display

uint32_t countdown_time;    // tracks the countdown time to bag date
typedef struct              // parses the countdown_time to its components
{
    uint8_t seconds, minutes, hours;
    uint16_t days;
} ElapsedTime_t;


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
 * setup the display, matrix, and button callbacks
 **/
void setup()
{
    // setup matrix display
    matrix.begin();
    matrix.setTextWrap(false);
    matrix.setBrightness(40);
    matrix.setTextColor(colors[0]);

    // setup the button event loops
    mode_button = new DebounceEvent(MODE_BUTTON_PIN, event_callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
    inc_button = new DebounceEvent(INC_BUTTON_PIN, event_callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);
    dec_button = new DebounceEvent(DEC_BUTTON_PIN, event_callback, BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP);

    /**
     * Initialize the countdown_time -- specify bag date & current date to kickoff the countdown_time
     * TODO: use str format for creating date -- https://forum.arduino.cc/index.php?topic=465881.0
            -- YYYY, MM, DD, HH, MM, SS
     */
    time_t bag_date = tmConvert_t(2019, 02, 19, 00, 00, 00);
    time_t now_date = tmConvert_t(2019, 01, 18, 9, 58, 00);
    countdown_time = bag_date - now_date;

    // TODO: use arduino time library for count down
    // settime(now_date);

#if DEBUG_ON
    Serial.print("bag_date: "); Serial.println(bag_date);
    Serial.print("now_date: "); Serial.println(now_date);
    Serial.print("countdown_time: "); Serial.println(countdown_time);
#endif

    // initialize state machine
    state = STATE_COUNTDOWN;
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
    every(1000)
    {
        state = state_machine(state, EVENT_TIMER);
    }
}

int x = matrix.width();
int pass = 0;

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
/**
 * Scrolling message
 * TODO: fix the display configuration to support the length of the message
 * and font size
 */
void print_scroll(const char *str)
{
    matrix.fillScreen(0);
    matrix.setCursor(x, 0);
    matrix.print(F(str));
    /** starts the print from width of the matrix + offset, which is 36
     * when offset is reached, pass is complete & color is updated
     * TODO: fix the offset calculation
    **/
    int offset = (strlen(str)*4) + 36;
    if (--x < -offset)
    {
        x = matrix.width();
        if (++pass >= 3)
            pass = 0;
        matrix.setTextColor(colors[pass]);
    }
    matrix.show();
}

/**
 * Print time starting at 0,0 origin
 * TODO: change color of the display text based on state mode
 */
//void print_time(uint16_t dd, uint8_t hh, uint8_t mm, uint8_t ss)
void print_time(ElapsedTime_t *t)
{
    char str[11];

    // build the passed in arguments as string
    sprintf(str, "%u:%d:%d:%d", t->days, t->hours, t->minutes, t->seconds);
#if DEBUG_ON
    Serial.print("Clock: |"); Serial.print(str); Serial.println("|");
#endif
    matrix.fillScreen(0);
    matrix.setCursor(0, 0);
    matrix.setTextColor(colors[1]);
    matrix.print(F(str));
    matrix.show();
}

int current_color=0;
// TODO: fix the code for color selection
// FIXME: this code does NOT work!
void change_text_color()
{
    int next = current_color;
    if (next == 3)
    {
        next = 0;
    }

    matrix.setTextColor(colors[++next]);
    matrix.show();
    current_color = next;
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
#define ONE_DAY     (24 /* hrs */ * 60 /* mins */ * 60 /* sec */)
#define ONE_HOUR    (60 /* mins */ * 60 /* sec */)
#define ONE_MINUTE  (60 /* sec */)
#define ONE_SECOND (1 /* sec */)
State_t state_machine(State_t current_state, Event_t event)
{
    State_t next_state = current_state;

    if (event == EVENT_MODE)
    {
        next_state = advance_state(current_state);
    }
    else
    {
        switch (current_state)
        {
            case STATE_COUNTDOWN:   // Fall through
            case STATE_MESSAGE:
                if (event == EVENT_TIMER)
                {
                    countdown_time = countdown_time - 1;
                }
                break;

            case STATE_DAYS:
                change_text_color();
                if (event == EVENT_INCREMENT)
                {
                    // increase the day count
                    countdown_time += ONE_DAY;
                }
                else if (event == EVENT_DECREMENT)
                {
                    countdown_time -= ONE_DAY;
                }
                break;

            case STATE_HOURS:
                change_text_color();
                if (event == EVENT_INCREMENT)
                {
                    // increase the day count
                    countdown_time += ONE_HOUR;
                }
                else if (event == EVENT_DECREMENT)
                {
                    countdown_time -= ONE_HOUR;
                }
                break;

            case STATE_MINUTES:
                change_text_color();
                if (event == EVENT_INCREMENT)
                {
                    // increase the day count
                    countdown_time += ONE_MINUTE;
                }
                else if (event == EVENT_DECREMENT)
                {
                    countdown_time -= ONE_MINUTE;
                }
                break;

            case STATE_SECONDS:
                change_text_color();
                if (event == EVENT_INCREMENT)
                {
                    // increase the day count
                    countdown_time += ONE_SECOND;
                }
                else if (event == EVENT_DECREMENT)
                {
                    countdown_time -= ONE_SECOND;
                }
                break;

            // unhandled case -- don't know how we got here, so just set the default
            default:
                next_state = STATE_MESSAGE;
                break;
        }
    }

    // update the display based on state
    do_clock(next_state);

    return next_state;
}

/**
 * Updates the display according to state instructions
 */
void do_clock(State_t state)
{
    switch (state)
    {
        case STATE_MESSAGE:
            // Print our message
            print_scroll(message);
            break;
        case STATE_COUNTDOWN:
        default:
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
            print_time(&elapsed_time);
            break;
    }
}