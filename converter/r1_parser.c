#include <stdint.h>
#include <string.h>

#include "adb_side.h"
#include "state.h"

// Parser for "talk register 1" messages
void handle_r1_message (uint8_t * msg) {
  tablet.max_x = ((uint16_t)(msg[2]) << 8) | msg[3];
  tablet.max_y = ((uint16_t)(msg[4]) << 8) | msg[5];

  switch (tablet.max_x) {
  case 0x319c:
    tablet.id = GD_0405_A; break;
  case 0x4f60:
    tablet.id = GD_0608_A; break;
  case 0x594c:
    tablet.id = GD_0912_A; break;
  case 0x7710:
    tablet.id = GD_1212_A; break;
  default:
    tablet.id = GD_OTHER; break;
  }
    
}
