#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>

uint8_t signal_nybble(uint8_t value);
void signal_internybble ();
void signal_pause ();
uint8_t signal_byte_bcd(uint8_t value, uint8_t signal_all_nybbles);
uint8_t signal_word_bcd(uint16_t word, uint8_t signal_all_nybbles);
void error_condition(uint8_t code);

#endif
