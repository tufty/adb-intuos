#ifndef __ADB_SIDE_H__
#define __ADB_SIDE_H__

#include <stdint.h>

void handle_r1_message(uint8_t msg_length, uint8_t * msg);
void handle_r0_message(uint8_t msg_length, uint8_t * msg);

#endif
