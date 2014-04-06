#include <stdint.h>
#include <string.h>

#include "adb_side.h"
#include "usb_side.h"
#include "state.h"

// Parser for "talk register 1" messages
void handle_r1_message (uint8_t msg_length, uint8_t * msg) {
  if (msg_length < 8) {
    error_condition(1);
  }

  uint16_t id_product = 0xdead;
  uint16_t max_x = ((uint16_t)(msg[2]) << 8) | msg[3];
  uint16_t max_y = ((uint16_t)(msg[4]) << 8) | msg[5];

  // Tablet ids taken from http://www.linux-usb.org/usb.ids
  // Little-endian
  switch (tablet.max_x) {
  case 0x319c:
    id_product = 0x4100;    // Intuos 2 4x5
    string2[8] = '0'; string2[10] = '4'; string2[12] = '0'; string2[14] = '5';
    //    id_product = 0xb000;    // Intuos 3 4x5
    break;
  case 0x4f60:
    id_product = 0x4200;    // Intuos 2 6x8
    string2[8] = '0'; string2[10] = '6'; string2[12] = '0'; string2[14] = '8';
   //    id_product = 0xb100;    // Intuos 3 6x8
    break;
  case 0x594c:
    id_product = 0x4300;    // Intuos 2 9x12
    string2[8] = '0'; string2[10] = '9'; string2[12] = '1'; string2[14] = '2';
    //    id_product = 0xb200;    // Intuos 3 9x12
    break;
  case 0x7710:
    string2[8] = '1'; string2[10] = '2'; string2[12] = '1'; 
    if (max_y == max_x) {
      id_product = 0x4400;  // Intuos 2 12x12
      string2[14] = '2';
     //      id_product = 0xb300;  // Intuos 3 12x12
    } else { 
      id_product = 0x4800;  // Intuos 2 12x18
      string2[14] = '8';
      //      id_product = 0xb400;  // Intuos 3 12x18
    }
    break;
  default:
    // Error 1 - Failed to recognise tablet
    error_condition(2);
    break;
  }

  // Populate device descriptor
  device_descriptor[10] = (uint8_t)(id_product >> 8);
  device_descriptor[11] = (uint8_t)(id_product & 0xff);
}
