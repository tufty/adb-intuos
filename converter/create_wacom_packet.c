#include <stdint.h>
#include <string.h>

#include "state.h"
#include "usb_side.h"

wacom_report_t usb_report;

void populate_in_proximity(uint8_t index, wacom_report_t * packet) {
  // zero everything
  memset(packet, 0, sizeof(wacom_report_t));
  packet->bytes[0] = 0x02;

  // enter proximity packet type
  packet->bytes[1] = 0xc2 | (index & 0x01);
  
  // Transducers id
  // This is a 24 bit value, bits 12-15 are zero
  // bits 12-23 ony exist for intuos / cintiq models after intuos 4
  packet->bytes[7] = transducers[index].type >> 20;           // bits 20-23
  packet->bytes[8] = (transducers[index].type >> 12) & 0xf0;  // bits 16-19
  packet->bytes[2] = transducers[index].type >> 4;            // bits 4-11
  packet->bytes[3] = transducers[index].type << 4;            // bits 0-3

  // Transducers serial number
  // 32 bit value
  packet->bytes[3] |= (transducers[index].id >> 28) & 0x0f;  // bits 28-31
  packet->bytes[4] = transducers[index].id >> 20;            // bits 20-27
  packet->bytes[5] = transducers[index].id >> 12;            // bits 12-19
  packet->bytes[6] = transducers[index].id >> 4;             // bits 4-11
  packet->bytes[7] |= transducers[index].id << 4;            // bits 0-3
}

void populate_out_of_proximity(uint8_t index, wacom_report_t * packet) {
  // zero everything
  memset(packet, 0, sizeof(wacom_report_t));
  packet->bytes[0] = 0x02;

  // exit proximity packet type
  packet->bytes[1] = 0x80 | (index & 0x01);
}

void populate_update(uint8_t index, wacom_report_t * packet) {
  // zero everything
  memset(packet, 0, sizeof(wacom_report_t));
  packet->bytes[0] = 0x02;

  packet->tool_index = index;
  packet->proximity = transducers[index].touching;

  // Intuos 3 has twice the resoution of intuos 1, so multiply numbers by 2
  packet->x = transducers[index].location_x << 1;
  packet->y = transducers[index].location_y << 1;

  // Now we've done the generic update stuff, time to go tool-specific
  switch (transducers[index].type) {
  case STYLUS_STANDARD:
    // populate pen buttons
    packet->bytes[1] |= (transducers[index].buttons & 0x03) << 1;
    // And the mysterious top 2 bits
    packet->bytes[1] |= 0xc0;

    // The pen specific stuff
    packet->pen_payload.tilt_x = transducers[index].tilt_x;
    packet->pen_payload.tilt_y = transducers[index].tilt_y;
    packet->pen_payload.pressure = transducers[index].pressure;

    if (transducers[index].touching) {
      packet->pen_payload.distance = 0x0d;
    } else {
      packet->pen_payload.distance = 0x1f;
    }
    break;
  case MOUSE_4D:
    // I think this means 4d mouse.
    packet->bytes[1] |= 0xc8;

    packet->mouse_4d_payload.rotation = transducers[index].rotation;
    packet->mouse_4d_payload.buttons = transducers[index].buttons & 0x3f;
    // Do this better, it's probably fucked, the wacom setup is complex
    // May actually be closer to the raw value from the tablet (see also rotation)
    packet->mouse_4d_payload.z = transducers[index].z >> 2;
    break;
  default:
    break;
  }
}    

void queue_message(message_type_t type, uint8_t index) {
  switch (type) {
  case TOOL_IN:
    populate_in_proximity(index, &usb_report);
    break;
  case TOOL_OUT:
    populate_out_of_proximity(index, &usb_report);
    break;
  case TOOL_UPDATE:
    populate_update(index, &usb_report);
    break;
  default:
    break;
  }
  usb_send_packet(usb_report.bytes, 10, WACOM_INTUOS5_PEN_ENDPOINT, 50);
}
