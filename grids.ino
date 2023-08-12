//  ============================================================
//
//  Program: Port of Mutable Instruments Grids to ArdCore
//
//  Description: This sketch provides a gate sequence based
//               on an array kept in memory. The output is
//               sent out the 8-bit output, but is primarily
//               used with the output expander.
//
//  I/O Usage:
//    Knob 1: X coordinate in drum map
//    Knob 2: Y coordinate in drum map
//    Analog In 1: Fill amount
//    Analog In 2: Randomness amount
//    Digital Out 1: BD trigger
//    Digital Out 2: SD trigger
//    Clock In: External clock input
//    Analog Out: HH trigger
//
//  Input Expander: unused
//  Output Expander: unused
//
//  Created:  06 Aug 2023
//
//  ============================================================
//
//  License:
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//  ================= start of global section ==================

#define __PROG_TYPES_COMPAT__
#include  <avr/pgmspace.h>
typedef uint8_t PROGMEM prog_uint8_t;
#include "patterns.h"
#include "avrlib/random.h"

using namespace avrlib;

uint16_t Random::rng_state_ = 0x21;

//  constants related to the Arduino Nano pin use
const int clkIn = 2;           // the digital (clock) input
const int digPin[2] = {3, 4};  // the digital output pins
const int pinOffset = 5;       // the first DAC pin (from 5-12)
const int trigTime = 10;       // 10 ms trigger time

const int PATTERN_LENGTH = 32;  
const int NUMBER_OF_PARTS = 3;

// analog in pins
const int X = 0;
const int Y = 1;
const int FILL = 2;
const int RANDOMNESS = 3;

int fillAmount;

//  variables for interrupt handling of the clock input
volatile int clkState = LOW;
int step = 0;

/* static */
uint8_t partPerturbations[NUMBER_OF_PARTS];

// timer variables
unsigned long clkMilli = 0;
int trigState = 0;

static const prog_uint8_t* patterns[5][5] = {
  { node_10, node_8, node_0, node_9, node_11 },
  { node_15, node_7, node_13, node_12, node_6 },
  { node_18, node_14, node_4, node_5, node_3 },
  { node_23, node_16, node_21, node_1, node_2 },
  { node_24, node_19, node_17, node_20, node_22 },
};

uint8_t randomNumber = 0;

/**
  Morphing function between 2 bytes taken from MI avrlib
  https://github.com/pichenettes/avril/blob/master/op.h#L644
*/
static inline uint8_t U8Mix(uint8_t a, uint8_t b, uint8_t balance) {
  return a * (255 - balance) + b * balance >> 8;
}

/**
  Taken from Mutable Instruments' avrlib
  https://github.com/pichenettes/avril/blob/master/op.h#L335
*/
static inline uint8_t U8U8MulShift8(uint8_t a, uint8_t b) {
  uint8_t result;
  asm(
    "mul %1, %2"      "\n\t"
    "mov %0, r1"      "\n\t"
    "eor r1, r1"      "\n\t"
    : "=r" (result)
    : "a" (a), "a" (b)
  );
  return result;
}

/* static */
uint8_t readDrumMap(
    uint8_t step,
    uint8_t instrument,
    uint8_t x,
    uint8_t y) {
  uint8_t i = x >> 6;
  uint8_t j = y >> 6;
  const prog_uint8_t* a_map = patterns[i][j];
  const prog_uint8_t* b_map = patterns[i + 1][j];
  const prog_uint8_t* c_map = patterns[i][j + 1];
  const prog_uint8_t* d_map = patterns[i + 1][j + 1];
  uint8_t offset = (instrument * PATTERN_LENGTH) + step;
  uint8_t a = pgm_read_byte(a_map + offset);
  uint8_t b = pgm_read_byte(b_map + offset);
  uint8_t c = pgm_read_byte(c_map + offset);
  uint8_t d = pgm_read_byte(d_map + offset);
  return U8Mix(U8Mix(a, b, x << 2), U8Mix(c, d, x << 2), y << 2);
}

void setup() {
  // set up the digital (clock) input
  pinMode(clkIn, INPUT);
  
  // set up the digital outputs
  for (int i=0; i<2; i++) {
    pinMode(digPin[i], OUTPUT);
    digitalWrite(digPin[i], LOW);
  }
  
  // set up the 8-bit DAC output pins
  for (int i=0; i<8; i++) {
    pinMode(pinOffset+i, OUTPUT);
    digitalWrite(pinOffset+i, LOW);
  }

  // set up the interrupt  
  attachInterrupt(0, isr, RISING);
}

void loop()
{  
  int clock = 0;
  
  if (clkState == HIGH) {
    clock = 1;
    clkState = LOW;
  }
  
  if (clock) {
    // update the time
    clkMilli = millis();

    // update random generator state
    Random::Update();
          
    // reduce the resolution of input values from 10 bits to 8 bits (0-255)
    uint8_t x = analogRead(X) >> 2;
    uint8_t y = analogRead(Y) >> 2;
    uint8_t fillControl = analogRead(FILL) >> 2;

    if (step == 0) {
      uint8_t randomness = analogRead(RANDOMNESS) >> 4; 

      for (uint8_t i = 0; i < NUMBER_OF_PARTS; ++i) {
        partPerturbations[i] = U8U8MulShift8(Random::GetByte(), randomness);
      }
    }

    // loop through drums: BD = 0, SD = 1, HH = 2
    for (int i=0; i<NUMBER_OF_PARTS; i++) {
      // fill control affects all 3 drums by default
      // change to `if (i = 0)` if you want fill to only affect BD
      // or `if (i <= 1)` if you want fill to affect only BD and SD
      if (i <= 2) {
        // flip 0-255 so that fill increases as you turn the knob CW
        fillAmount = 255 - fillControl;
      } else {
        fillAmount = 128; // hardcoded default fallback
      }

      int drumThreshold = readDrumMap(step, i, x, y);

      if (drumThreshold < 255 - partPerturbations[i]) {
        drumThreshold += partPerturbations[i];
        
      } else {
        drumThreshold = 255;
      }

      bool outputTrig = drumThreshold > fillAmount;
      if (i < 2) {
        digitalWrite(digPin[i], outputTrig);
      } else {
        // use DAC output for the third drum HH
        dacOutput(outputTrig ? 255 : 0);
      }
    }

    // update the pointer
    step++;
    if (step >= PATTERN_LENGTH) {
      step = 0;
    }
    
    trigState = 1;
  }
  
  if (((millis() - clkMilli) > trigTime) && (trigState)) {
    trigState = 0;

    dacOutput(LOW);
    for (int i=0; i<2; i++) {
      digitalWrite(digPin[i], LOW);
    }
  }
}

//  =================== convenience routines ===================

//  dacOutput(byte) - deal with the DAC output
//  -----------------------------------------
void dacOutput(byte v)
{
  PORTB = (PORTB & B11100000) | (v >> 3);
	PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}

//  isr() - quickly handle interrupts from the clock input
//  ------------------------------------------------------
void isr()
{
  clkState = HIGH;
}

//  ===================== end of program =======================
