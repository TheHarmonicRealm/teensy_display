// Adafruit_NeoMatrix example for single NeoPixel Shield.
// Scrolls 'Howdy' across the matrix in a portrait (vertical) orientation.

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/FreeMono9pt7b.h>

#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define PIN 17

#define CHAR_HEIGHT 7

// 4x7 number digit font bitmaps
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


        // MATRIX DECLARATION:
        // Parameter 1 = width of NeoPixel matrix
        // Parameter 2 = height of matrix
        // Parameter 3 = pin number (most are valid)
        // Parameter 4 = matrix layout flags, add together as needed:
        //   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
        //     Position of the FIRST LED in the matrix; pick two, e.g.
        //     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
        //   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
        //     rows or in vertical columns, respectively; pick one or the other.
        //   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
        //     in the same order, or alternate lines reverse direction; pick one.
        //   See example below for these values in action.
        // Parameter 5 = pixel type flags, add together as needed:
        //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
        //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
        //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
        //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

        // Example for NeoPixel Shield.  In this application we'd like to use it
        // as a 5x8 tall matrix, with the USB port positioned at the top of the
        // Arduino.  When held that way, the first pixel is at the top right, and
        // lines are arranged in columns, progressive order.  The shield uses
        // 800 KHz (v2) pixels that expect GRB color data.
        Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(45, 7, PIN,
                                                       NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
                                                           NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                                                       NEO_GRB + NEO_KHZ800);

const uint16_t colors[] = {
      matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255)};

static int _cursor = 0;

void clear_screen(void)
{
  matrix.clear();
  matrix.show();
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
  matrix.show();
  _cursor += width;
}

void print_colon(uint16_t color)
{
  int width = 2; // Colon width

  matrix.fillRect(_cursor, 0, _cursor + width, CHAR_HEIGHT, 0);
  matrix.drawBitmap(_cursor, 0, colon_bitmap, width, CHAR_HEIGHT, color);
  matrix.show();
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
}

void setup()
{
    matrix.begin();
    matrix.setTextWrap(false);
    matrix.setBrightness(40);
    matrix.setTextSize(5);
    matrix.setTextColor(colors[0]);
}

int x = matrix.width();

void loop()
{
  print_countdown(12, 34, 56, 78);
  delay(5000);
  print_countdown(9, 1, 10, 88);
  delay(5000);
}
