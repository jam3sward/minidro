#include <FastPin.h>
#include <LedControl.h>

// Mini-DRO is a small DRO project for a mini lathe.
//
// It interfaces to two cheap digital calipers which have serial
// data output, and displays their position on two 8 digit LED
// 7 segment displays using the MAX7219 controller. The calipers
// send 28 bits of data, in groups of four bits. The first 5
// nibbles represent the 5 digits of the caliper display in BCD
// format, with the LSB being transmitted first.
//
// There are a lot of similar projects already on the internet,
// but none of the code I tried seemed to work with my calipers,
// so I wrote this pretty much from scratch.
//
// Required hardware:
//   2x 8 digit 7 segment MAX7219 LED display
//   2x Cheap Digital Calipers with BCD output format
// The 1.5V caliper output is converted to 5V with NPN
// transistors, which results in the signal being inverted,
// and this code is designed to handle this inverted data.
// 
// Requires the FastPin library:
//   https://github.com/jam3sward/fastpin
// Requires the LedControl library:
//   https://github.com/wayoda/LedControl
//
// James Ward 03/02/19

#include "Caliper.h"

const int LED_ROWS    = 2;  // Number of LED modules
const int LED_COLUMNS = 8;  // Number of columns per module

const int LED_DIN = 12;     // Data pin on first LED module
const int LED_CLK = 11;     // Clock pin on first LED module
const int LED_CS  = 10;     // Chip select on first LED module

int CAL1_DATA_PIN   = 9;    // Caliper data pin
int CAL1_CLOCK_PIN  = 8;    // Caliper clock pin

// initialise LedControl
LedControl led(LED_DIN, LED_CLK, LED_CS, LED_ROWS);

// custom "double quote" character for the 7 segment LED display
// (this is used to indicate inches)
const byte LED_DOUBLE_QUOTE = B00100010;

// initialise the calipers
// (I'm just testing one of the calipers at the moment, so this
// reads and displays the same caliper on two displays currently)
Caliper caliper1(CAL1_CLOCK_PIN, CAL1_DATA_PIN);
Caliper caliper2(CAL1_CLOCK_PIN, CAL1_DATA_PIN);

// initialisation
void setup()
{
  // for each LED display
  for (int i=0; i<led.getDeviceCount(); i++) {
    // wake up
    led.shutdown(i, false);
    // set medium intensity
    led.setIntensity(i, 4);
    // clear the display
    led.clearDisplay(i);
  }
}

// display the caliper value on a given row of the LED display
void display(byte row, const Caliper &caliper)
{
  // the least significant 20 bits represent 5 digits
  // in BCD format (4 bits per digit)
  uint32_t measure = caliper.getDigitsBCD();

  // is the value negative?
  bool negative = caliper.isNegative();

  // is the value in mm?
  bool mm = caliper.isMetric();

  // for inch mode, there's another "half bit" flag which
  // represents 5/1000
  bool half = caliper.isHalf();

  // the maximum number of digits we will output (for mm)
  // (this increases for inch)
  int digits = 5;

  // the number of digits after the decimal point (for mm)
  // (this increases for inch)
  int decimal = 2;

  // for inch mode, add the half digit directly into the value
  if (!mm) {
    measure = (measure << 4) | (half ? 5:0);
    // we will output 6 digits for inch (4 after the decimal)
    digits  = 6;
    decimal = 4;
  }

  // keeps track of horizontal position (from the right)
  int x=0;

  // output a double quote symbol when we're in inch mode
  if (!mm)
    led.setRow(row, x++, LED_DOUBLE_QUOTE);

  // output the digits from right to left, inserting
  // the decimal point and suppressing leading zeros
  uint32_t remain = measure;  
  for (int i=0; i<digits; i++) {
    if (i <= decimal || remain > 0) {
      // output one digit
      led.setDigit(row, x++, remain & 0xF, (i==decimal)?1:0);
      remain >>= 4;
    } else {
      // suppress leading zeros
      break;
    }
  }

  // output the negative sign (avoiding negative zero)
  if (negative && measure != 0)
    led.setChar(row, x++, '-', 0);

  // fill the remainder of the line with blanks
  for (; x<LED_COLUMNS; x++)
    led.setChar(row, x, ' ', 0);
}

// the main loop
void loop() {
  // get updates from both calipers and display their
  // current position on two LED displays
  if (caliper1.update()) display(0, caliper1);
  if (caliper2.update()) display(1, caliper2);
}
