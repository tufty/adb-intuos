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
