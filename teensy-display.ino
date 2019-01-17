/**
 * Spartronics sample code for Teensy LC based display via Adafruit NeoMatrix
 *
 * Parts:
 *  - Teensy LC
 *  - Adafruit NeoMatrix
 *
 * Usage:
 *  3 buttons control the behavior of the display
 *  - button 1: specifies mode for reprogramming the display
 *  - button 2 & 3: indicates increment/decrement values based on the mode
 */

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>

/**
 * Setup the NeoMatrix display and button controls
 *  - include debug code to verify hardware functionality
 * Step 1: setup NeoMatrix display
 * Step 2: setup 3 control buttons
 */

#define DEBUG_ON    FALSE

#define CONTROL_PIN 17      // used for the NeoMatrix configuration
#define MODE_BUTTON 14      // D14 (D == digital port) -- sets display mode
#define INC_BUTTON  15      // D15 -- increments value
#define DEC_BUTTON  16      // D16 -- decrements value


/** Good reference for display configuration:
 *  https://learn.adafruit.com/adafruit-neopixel-uberguide/neomatrix-library
 *
 *  Spartronics display is setup as a 45x7 matrix.The first pixel is on top left,
 *  lines are arranged in rows as zig-zag order. The shield uses 800Khz (v2)
 *  pixels that expect GRB color data
 */

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(45, 7,   // matrix width & height
  CONTROL_PIN,                                  // pin number
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +        // matrix layout, flags added as needed
  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);             // pixel type flags, added as needed

// define a structure to track display state and defaults
typedef enum {  // at each button mode press, display mode changes accordingly
    STATE_DAYS,
    STATE_HOURS,
    STATE_MINS,
    STATE_SEC,
    STATE_SAYINGS,
    STATE_COUNTDOWN,
    STATE_MAX
} State_t;

//typedef struct Display_t display;
typedef struct display {
    State_t state;  // display mode for controlling the setup
    const char *saying = { "Welcome to Spartronics Open House!" };
} Display_t;

const uint16_t colors[] = {
    matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255)};

// setup display
Display_t display = {
    .state = STATE_COUNTDOWN
};

void setup()
{
    // setup matrix display
    matrix.begin();
    matrix.setTextWrap(false);
    matrix.setBrightness(40);
    matrix.setTextColor(colors[0]);
}

void loop()
{
#ifdef DEBUG_ON
    print_display("Debug Msg!");
#endif

    print_display(display.saying);
}

//int x = matrix.width();
int x = strlen(display.saying);
int pass = 0;

// print a specified saying on the display
void print_display(const char *str)
{
    matrix.fillScreen(0);
    matrix.setCursor(x, 0);
    matrix.print(F(str));
    // change color once the string is displayed
    if (--x < -36)
    {
//        x = matrix.width();
        x = strlen(display.saying);
        if (++pass >= 3)
            pass = 0;
        matrix.setTextColor(colors[pass]);
    }
    matrix.show();
    delay(100);
}
