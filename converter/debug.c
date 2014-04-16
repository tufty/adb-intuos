// Copyright (c) 2013-2014 by Simon Stapleton (simon.stapleton@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "debug.h"
#include "led.h"
#include <util/delay.h>

uint8_t signal_nybble(uint8_t value) {
  if (value == 0) {
    LED_OFF; _delay_ms(500);
  } else {
    for(uint8_t i = 0; i < value; i++) {
      LED_ON;  _delay_ms(250);
      LED_OFF; _delay_ms(250);
    }
  }
  return value;
}

void signal_internybble () {
  for (uint8_t i = 0; i < 3; i++) {
    LED_ON;  _delay_ms(50);
    LED_OFF; _delay_ms(50);
  }
  _delay_ms(250);
}   

void signal_pause () {
  LED_OFF; _delay_ms(1000);
}

uint8_t signal_byte_bcd(uint8_t value, uint8_t signal_all_nybbles) {
  uint8_t nybble = value >> 4;
  if (nybble | signal_all_nybbles) {
    signal_all_nybbles |= signal_nybble(nybble);
    signal_internybble();
  }
  nybble = value & 0x0f;
  if (nybble | signal_all_nybbles) {
    signal_all_nybbles |= signal_nybble(nybble);
  }
  return signal_all_nybbles;
}

uint8_t signal_word_bcd(uint16_t word, uint8_t signal_all_nybbles) {
  uint8_t value = word >> 8;
  if (value | signal_all_nybbles) {
    signal_all_nybbles = signal_byte_bcd(value, signal_all_nybbles);
    signal_internybble();
  }
  value = word & 0xff;
  if (value | signal_all_nybbles) {
    signal_all_nybbles = signal_byte_bcd(value, signal_all_nybbles);
  }
  return signal_all_nybbles;
}

void error_condition(uint8_t code) {
  while (1) {
    signal_pause();
    signal_byte_bcd(code, 0);
  }
}
