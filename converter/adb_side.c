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

#include "adb_side.h"
#include "usb_side.h"
#include "led.h"
#include "shared_state.h"
#include "debug.h"

#include <string.h>

transducer_t transducers[2];
tablet_type_t tablet_family;
uint16_t max_x;
uint16_t max_y;

// Parser for "talk register 1" messages
void handle_wacom_r1_message (uint8_t msg_length, volatile uint8_t * msg) {
  if (msg_length == 0)
    error_condition(10);
  if (msg_length < 8) {
    error_condition(msg_length);
  }

  uint16_t id_product = 0xdead;
  max_x = ((uint16_t)(msg[2]) << 8) | (uint16_t)msg[3];
  max_y = ((uint16_t)(msg[4]) << 8) | (uint16_t)msg[5];

  // Tablet ids taken from http://www.linux-usb.org/usb.ids
  // Little-endian
  switch (max_x) {
  case 0x319c:
    id_product = 0x4100;    // Intuos 2 4x5
    break;
  case 0x4f60:
    id_product = 0x4200;    // Intuos 2 6x8
    break;
  case 0x594c:
    break;
  case 0x7710:
    if (max_y == max_x) {
      id_product = 0x4400;  // Intuos 2 12x12
    } else { 
      id_product = 0x4300;    // Intuos 2 9x12
    }
    break;
  default:
    id_product = 0x4800;  // Intuos 2 12x18
    break;
  }

  identify_product(id_product);
}

void handle_calcomp_r1_message(uint8_t msg_length, volatile uint8_t * msg) {
  switch (msg[1]) {
  case 0:
    // Calcomp 4x5
    id_product = 0x4100;    // Intuos 2 4x5
    break;
  case 1:
    // Calcomp 6x9
    id_product = 0x4200;    // Intuos 2 6x8
    break;
  case 2:
    // Calcomp 12x12
    id_product = 0x4400;    // Intuos 2 12x12
    break;
  case 3:
    // Calcomp 12x18
    id_product = 0x4800;    // Intuos 2 12x18
    break;
  }
 
  max_x = (msg[3] >> 1) * 1000;
  max_y = (msg[4] >> 1) * 1000;
  
}

void handle_r1_message(uint8_t msg_length, volatile uint8_t * msg) { 
  if (tablet_family != CALCOMP) {
    handle_wacom_r1_message(msg_length, msg);
  } else {
    handle_calcomp_r1_message(msg_length, msg);
  }
}

// Averaging of locations
void average_location(uint8_t average, uint16_t loc, uint16_t * p_loc, uint16_t * p_old_loc) {
  if (average) {
    *p_loc = (loc + *p_old_loc) >> 1;
  } else {
    *p_loc = loc;
  }
  *p_old_loc = loc;
}

// Process a location delta.  This one's a bit hairy
void process_location_delta (uint8_t raw, uint8_t * shift, uint16_t * location, uint16_t * old_location, uint16_t loc_max) {
  uint8_t magnitude = raw & 0x0f;
  uint16_t delta = magnitude << *shift;
  int32_t new_location = *old_location;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x10) {
    new_location -= delta;
  } else {
    new_location += delta;
  }

  // And the new shift value
  switch (magnitude) {
  case 0:
    new_shift -= 2; break;
  case 1: case 2: case 3: case 4: case 5: case 6: case 7:
    new_shift -= 1; break;
  case 15:
    new_shift += 2; break;
  default:
    break;
  }
  // Make sure the shift value is positive
  *shift = (new_shift > 0) ? new_shift : 0;

  if (new_location < 0) new_location = 0;
  if (new_location > loc_max) new_location = loc_max;

  // there's an averaging step here, that the other delta handlers don't have
  average_location(1, new_location, location, old_location);
}

void process_tilt_delta (uint8_t raw, uint8_t * shift, uint16_t * value) {
  uint8_t magnitude = raw & 0x07;
  uint16_t delta = magnitude << *shift;
  int16_t new_value = *value;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x08) {
    new_value -= delta;
  } else {
    new_value += delta;
  }
  switch (magnitude) {
  case 0x0:
    new_shift -= 3; break;
  case 0x1:
    new_shift -= 2; break;
  case 0x2:
  case 0x3:
    new_shift -= 1; break;
  case 0x6:
    new_shift += 1; break;
  case 0x7:
    new_shift += 2; break;
  default:
    break;
  }
  // Make sure the shift value is positive
  *shift = (new_shift > 0) ? new_shift : 0;
  
  if (new_value < 0) new_value = 0;
  if (new_value > 0x7f) new_value = 0x7f;

  *value = new_value;
}

void process_pressure_delta(uint8_t raw, uint8_t * shift, uint16_t * value, uint8_t * flag) {
  uint8_t magnitude = raw & 0x07;

  if (magnitude == 0x07) magnitude = 0x40;

  uint16_t delta = magnitude << *shift;
  int16_t new_value = *value;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x08) {
    new_value -= delta;
  } else {
    new_value += delta;
  }

  if (*flag) {
    if (raw & 0x08) {
      new_value = 0x09;
    } else {
      *flag = 0;
    }
  } else {
    if (raw == 0x0f) {
      new_value = 0x09;
      *flag = 1;
    }
  }

  switch (magnitude) {
  case 0x0:
    new_shift -= 3; break;
  case 0x1:
    new_shift -= 2; break;
  case 0x2:
  case 0x3:
    new_shift -= 1; break;
  case 0x6:
    new_shift += 1; break;
  case 0x7:
    new_shift += 2; break;
  default:
    break;
  }
  // Make sure the shift value is positive
  *shift = (new_shift > 0) ? new_shift : 0;

  if (new_value < 0) new_value = 0;
  if (new_value > 0x3ff) new_value = 0x3ff;

  *value = new_value;
}

void process_rotation_delta (uint8_t raw, uint8_t * shift, uint16_t * value) {
  uint8_t magnitude = raw & 0x03;
  uint16_t delta = magnitude << *shift;
  uint16_t new_value = *value;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x04) {
    new_value -= delta;
  } else {
    new_value += delta;
  }
  switch (magnitude) {
  case 0x0:
    new_shift -= 1; break;
  case 0x1:
    break;
  case 0x2:
    new_shift += 1; break;
  case 0x3:
    new_shift += 2; break;
  }
  // Make sure the shift value is positive
  *shift = (new_shift > 0) ? new_shift : 0;

  if (new_value < 0) new_value = 0;
  if (new_value > 0xe0e) new_value = 0xe0e;

  *value = new_value;
}


void handle_gd_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
  uint8_t index = 0;
  uint8_t transducer = 0;
  while (index < msg_length) {
    uint8_t send_message = 1;
    // switch on message major type
    switch (msg[index] & 0xf0) {
    case 0xf0:
      // Get the transducer number
      transducer = msg[index] & 0x01;
      // switch on subtype of message
      switch (msg[index] & 0x0e) {
      case 0x0e:     // out of proximity message
	// queue out of prox message and blat the transducer
	queue_message (TOOL_OUT, transducer);
	memset((void*)&(transducers[transducer]), 0, sizeof(transducer_t));
	index += 2;
	send_message = 0;
	break;
      case 0x0c:     // 7 byte packet, location and rotation - first packet from a 4d mouse
	transducers[transducer].state = 0xaa;
	transducers[transducer].touching = (msg[index + 5] & 0x80) >> 7;
	// Average locations if necessary
	average_location (transducers[transducer].entered_proximity ? 0 : 1, 
			  ((uint16_t)(msg[index + 1]) << 8) | msg[index + 2],
			  &transducers[transducer].location_x,
			  &transducers[transducer].location_x_old);
	average_location (transducers[transducer].entered_proximity ? 0 : 1, 
			  ((uint16_t)(msg[index + 3]) << 8) | msg[index + 4],
			  &transducers[transducer].location_y,
			  &transducers[transducer].location_y_old);

	// Rotation is reported here as a 10 bit value and sign.
	transducers[transducer].rotation = ((uint16_t)((msg[index + 5]) & 0x7f) << 3) | (msg[index + 6] >> 5);
	transducers[transducer].rotation_sign = msg[index + 6] & 0x10;

	// Delta operates on raw magnitude.  Need to deal with sign flips, probably
	// Only normalise for export to USB handler.

	// if (transducers[transducer].rotation_sign) {
	//   transducers[transducer].rotation = 0x707 - transducers[transducer].rotation;
	// }

	transducers[transducer].location_x_shift = 4;
	transducers[transducer].location_y_shift = 4;
	transducers[transducer].rotation_shift = 0;

	index += 7;
	break;
      case 0x0a:     // 8 byte packet, location and 24 bit button mask, probably puck
      case 0x08:
	/* transducers[transducer].state = 0xb0; */
	/* transducers[transducer].touching = (msg[index] & 0x02) >> 1; */
	/* set_location (transducer, (msg[index + 1] << 8) | msg[index + 2], (msg[index + 3] << 8) | msg[index + 4]); */
	/* set_buttons (transducer, (msg[index + 5] << 16) | (msg[index + 6] << 8) | msg[index + 7]); */
	send_message = 0;
	index += 8;
	break;
      case 0x04:     // 8 byte packet, location, tilt and ??? $1f6.b
      case 0x06:
	/* transducers[transducer].state = 0xac; */
	/* transducers[transducer].touching = (msg[index] & 0x02) >> 1; */
	/* set_location (transducer, (msg[index + 1] << 8) | msg[index + 2], (msg[index + 3] << 8) | msg[index + 4]); */
	/* set_1f6 (transducer,  (msg[index + 5] << 2) | (msg[index + 6] >> 6)); */
	/* set_tilt (transducer, ((msg[index + 6] << 1) | (msg[index + 7] >> 7)) & 0x7f, (msg[index + 7] & 0x7f)); */
	send_message = 0;
	index += 8;
	break;
      default:       // 8 byte packet, location, z, and 7 bit button mask - second packet for a 4d mouse
	transducers[transducer].state = 0xa8;
	transducers[transducer].touching = (msg[index] & 0x02) >> 1;

	average_location (transducers[transducer].entered_proximity ? 0 : 1, 
			  ((uint16_t)(msg[index + 1]) << 8) | msg[index + 2],
			  &transducers[transducer].location_x,
			  &transducers[transducer].location_x_old);
	average_location (transducers[transducer].entered_proximity ? 0 : 1, 
			  ((uint16_t)(msg[index + 3]) << 8) | msg[index + 4],
			  &transducers[transducer].location_y,
			  &transducers[transducer].location_y_old);

	transducers[transducer].location_x_shift = 4;
	transducers[transducer].location_y_shift = 4;
	transducers[transducer].z_shift = 4;

	transducers[transducer].z = (msg[index + 5] << 2) | (msg[index + 6] >> 6);
	transducers[transducer].buttons = (msg[index + 7] & 0x7f);
	index += 8;
      }
      transducers[transducer].entered_proximity = 0;
      break;
    case 0xe0:       // 8 byte packet, location, tilt, pressure
      /* transducer = msg[index] & 0x01; */
      /* transducers[transducer].state = msg[index] & 0x08 ? 0xb4 : 0xbc; */
      /* transducers[transducer].touching = (msg[index] & 0x02) >> 1; */
      /* set_location (transducer, (msg[index + 1] << 8) | msg[index + 2], (msg[index + 3] << 8) | msg[index + 4]); */
      /* set_pressure (transducer, (msg[index + 5] << 2) | (msg[index + 6] >> 6)); */
      /* set_tilt (transducer, ((msg[index + 6] << 1) | (msg[index + 7] >> 7)) & 0x7f, (msg[index + 7] & 0x7f)); */
      send_message = 0;
      index += 8;
      break;
    case 0xd0:       // 6 byte packet, location, pressure
      /* transducer = (msg[index] & 0x04) >> 2; */
      /* transducers[transducer].state = 0xb8; */
      /* set_location (transducer, (msg[index + 1] << 8) | msg[index + 2], (msg[index + 3] << 8) | msg[index + 4]); */
      /* set_pressure (transducer, (msg[index + 5] << 2) | (msg[index] & 0x03)); */
      send_message = 0;
      index += 6;
      break;
    case 0xc0:       // 6 byte packet, location, pressure	
      /* transducer = msg[index] & 0x01; */
      /* transducers[transducer].state = 0xb2; */
      /* transducers[transducer].touching = (msg[index] & 0x02) >> 1; */
      /* set_location (transducer, (msg[index + 1] << 8) | msg[index + 2], (msg[index + 3] << 8) | msg[index + 4]); */
      /* set_pressure (transducer, (msg[index + 5] << 2) | ((msg[index] >> 1) & 0x03)); */
      send_message = 0;
      index += 6;
      break;
    case 0xb0:       // 6 byte packet, location and ??? $1f6.b
      /* transducer = (msg[index] & 0x04) >> 2; */
      /* transducers[transducer].state = 0xae; */
      /* transducers[transducer].touching = (msg[index] & 0x02) >> 1; */
      /* set_location (transducer, (msg[index + 1] << 8) | msg[index + 2], (msg[index + 3] << 8) | msg[index + 4]); */
      /* set_if6 (transducer, (msg[index + 5] << 2) | (msg[index] & 0x03)); */
      send_message = 0;
      index += 6;
      break;
    case 0xa0:       // 8 byte packet, location, tilt, pressure, 2 bit buttons : Stylus major packet
      transducer = (msg[index] & 0x01);
      transducers[transducer].state = 0xa0;
      transducers[transducer].touching = (msg[index] & 0x02) >> 1;

      average_location (transducers[transducer].entered_proximity ? 0 : 1, 
			((uint16_t)(msg[index + 1]) << 8) | msg[index + 2],
			&transducers[transducer].location_x,
			&transducers[transducer].location_x_old);
      average_location (transducers[transducer].entered_proximity ? 0 : 1, 
			((uint16_t)(msg[index + 3]) << 8) | msg[index + 4],
			&transducers[transducer].location_y,
			&transducers[transducer].location_y_old);

      transducers[transducer].pressure = ((uint16_t)(msg[index + 5]) << 2) | (msg[index + 6] >> 6);
      transducers[transducer].tilt_x = ((msg[index + 6] << 1) | (msg[index + 7] >> 7)) & 0x7f;
      transducers[transducer].tilt_y = msg[index + 7] & 0x7f;
      transducers[transducer].buttons = (msg[index] & 0x06) >> 1;

      transducers[transducer].location_x_shift = 4;
      transducers[transducer].location_y_shift = 4;
      transducers[transducer].tilt_x_shift = 2;
      transducers[transducer].tilt_y_shift = 2;
      transducers[transducer].pressure_shift = 3;

      transducers[transducer].entered_proximity = 0;
      index += 8;
      break;
    case 0x90:       // 7 byte packet, tool type and tool serial number (initial proximity packet)
    case 0x80:
      // extract transducer specific data
      transducer = (msg[index] & 0x10) >> 4;
      transducers[transducer].state = 0xc2;
      transducers[transducer].type = ((uint16_t)msg[index + 1] << 4) | (msg[index + 2] >> 4);
      transducers[transducer].id = ((uint32_t)msg[index + 2] << 28) | ((uint32_t)msg[index + 3] << 20) | ((uint32_t)msg[index + 4] << 12) | ((uint32_t)msg[index + 5] << 4) | (msg[index + 6] >> 4);
      // initialise transducer shift values etc
      transducers[transducer].entered_proximity = 1;
      transducers[transducer].location_x_shift = 4;
      transducers[transducer].location_y_shift = 4;
      transducers[transducer].tilt_x_shift = 2;
      transducers[transducer].tilt_y_shift = 2;
      transducers[transducer].pressure_shift = 3;
      transducers[transducer].z_shift = 3;
      transducers[transducer].barrel_pressure_shift= 3;
      transducers[transducer].rotation_shift = 0;
      transducers[transducer].output_state = 0;
      transducers[transducer].rotation_sign = 0;
      transducers[transducer].pressure_sign = 0;
      transducers[transducer].z_sign = 0;
      index += 7;
      queue_message(TOOL_IN, transducer);
      send_message = 0;
      break;
    default:         // delta packet(s), 2 or 3 bytes, depends on preceding "major" packet
      transducer = (msg[index] & 0x40) >> 6;
      transducers[transducer].entered_proximity = 0;
      // All deltas have a location component
      process_location_delta (((msg[index] & 0x3e) >> 1), 
			      &transducers[transducer].location_x_shift,
			      &transducers[transducer].location_x,
			      &transducers[transducer].location_x_old,
			      max_x);
      process_location_delta (((msg[index] & 0x01) << 4) | (msg[index + 1] >> 4),
			      &transducers[transducer].location_y_shift,
			      &transducers[transducer].location_y,
			      &transducers[transducer].location_y_old,
			      max_y);

      // Now switch based on transducer type.
      switch (transducers[transducer].type & 0xff7) {
      case STYLUS_STANDARD:
	// this must be an stylus major packet (that's all we get)
	process_pressure_delta(msg[index + 1] & 0x0f, 
			       &transducers[transducer].pressure_shift,
			       &transducers[transducer].pressure,
			       &transducers[transducer].pressure_sign);

	// Deal with touching / not touching
	if (transducers[transducer].pressure == 0) {
	  transducers[transducer].touching = 0;
	} else {
	  transducers[transducer].touching = 1;
	}

	index += 2;
	if (index < msg_length) {
	  process_tilt_delta (msg[index] >> 4,
			      &transducers[transducer].tilt_x_shift,
			      &transducers[transducer].tilt_x);
	  process_tilt_delta (msg[index] & 0x0f,
			      &transducers[transducer].tilt_y_shift,
			      &transducers[transducer].tilt_y);
	  index += 1;
	} 
	break;
      case MOUSE_4D:
	// 4D mouse has 2 states
	if (transducers[transducer].state == 0xaa) {
	  process_pressure_delta(msg[index + 1] & 0x0f,
				 &transducers[transducer].z_shift,
				 &transducers[transducer].z,
				 &transducers[transducer].z_sign);
	  transducers[transducer].state = 0xa8;
	} else /* a8 */ {
	  process_rotation_delta((msg[index + 1] & 0x0e) >> 1,
				 &transducers[transducer].rotation_shift,
				 &transducers[transducer].rotation);
	  transducers[transducer].rotation_sign = msg[index + 1] & 0x01;

	  // Again, we should only normalise for output, or the delta calcs are screwed up
	  // if (transducers[transducer].rotation_sign) {
	  //   transducers[transducer].rotation = 0x707 - transducers[transducer].rotation;
	  // }
	  transducers[transducer].state = 0xaa;
	}
	index += 2;
	break;
      case STYLUS_INKING:
      case STYLUS_STROKE:
      case STYLUS_GRIP:
      case STYLUS_AIRBRUSH:
      case MOUSE_2D:
      case CURSOR:
	index += 8;  // ignore packet
	send_message = 0;
	break;
      }

      /*   switch (transducers[transducer].state) { */
      /*   case 0xa0: */
      /*   case 0xa8: */
      /* 	process_pressure_delta(transducer, msg[index + 1] & 0x0f); */
      /* 	index += 2; */
      /* 	break; */
      /*   case 0xaa: */
      /* 	process_wheel_delta(transducer, msg[index + 1] & 0x0f); */
      /* 	index += 2; */
      /*   case 0xac: */
      /* 	index += 2; */
      /* 	if (index + 2 < msg_length) { */
      /* 	  process_tilt_delta(transducer, msg[index + 2] >> 4, msg[index + 2] & 0x0f); */
      /* 	  index += 1; */
      /* 	}  */
      /* 	break; */
      /*   case 0xae: */
      /* 	index += 2; */
      /* 	break; */
      /*   case 0xb0: */
      /* 	index += 2; */
      /* 	break; */
      /*   case 0xb2: */
      /* 	process_pressure_delta(transducer, msg[index + 1] & 0x0f); */
      /* 	index += 2; */
      /* 	break; */
      /*   case 0xb4: */
      /* 	process_pressure_delta(transducer, msg[index + 1] & 0x0f); */
      /* 	index += 2; */
      /* 	if (index + 2 < msg_length) { */
      /* 	  process_tilt_delta(transducer, msg[index + 2] >> 4, msg[index + 2] & 0x0f); */
      /* 	  index += 1; */
      /* 	}  */
      /* 	break; */
      /*   case 0xb8: */
      /* 	process_pressure_delta(transducer, msg[index + 1] & 0x0f); */
      /* 	index += 2; */
      /* 	break; */
      /*  case 0xbc: */
      /* 	process_pressure_delta(transducer, msg[index + 1] & 0x0f); */
      /* 	index += 2; */
      /* 	if (index + 2 < msg_length) { */
      /* 	  process_tilt_delta(transducer, msg[index + 2] >> 4, msg[index + 2] & 0x0f); */
      /* 	  index += 1; */
      /* 	}  */
      /* 	break; */
      /*   } */
      /* } */

    }
    if (send_message == 1) {
      queue_message(TOOL_UPDATE, transducer);
    } 
  }
}

// Handler for Ultrapad r0 message
void handle_ud_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
  uint8_t index = 0;

  if (msg_length == 5) {
    // This is an older ultrapad sending 5-byte, no-tilt messages
    if (transducers[index].type == 0) {
      // Coming into proximity
      transducers[index].type = STYLUS_STANDARD;
      transducers[index].id = 0x16002cd;  // Bernard's Intuos 2 pen
      queue_message(TOOL_IN, index);
    }

    if (msg[0] & 0x80) {
      transducers[index].touching = msg[2] & 0x40 >> 6;
      transducers[index].location_x = (((uint16_t)msg[0] & 0x3f) << 8) | msg[1];
      transducers[index].location_y = (((uint16_t)msg[2] & 0x3f) << 8) | msg[3];
      transducers[index].pressure = msg[4] & 0x7f;
      transducers[index].buttons = (msg[2] & 0x80) >> 7;
      
      queue_message(TOOL_UPDATE, index);
    } else {
      queue_message(TOOL_OUT, index);
      memset((void*)&(transducers[index]), 0, sizeof(transducer_t));
    }
  } else {
    // Ultrapad returning a full 8 byte packet.
    // Not sure about this, only the 1212 model supports dual track anyway.
    // Seems to be the only bit that's free, so must be tool id, right?
    index = msg[0] & 0x80 ? 0 : 1;
  
    switch (msg[0] & 0x40) {
    case 0x40:
      // Proximity flag is set, this is a tool report
      if (transducers[index].type == 0) {
	// Entering proximity.  Identify the tool type.
	if (msg[0] & 0x20) {
	  if (msg[0] & 0x04) {
	    transducers[index].type = STYLUS_STANDARD_INVERTED;
	  } else {
	    transducers[index].type = STYLUS_STANDARD;
	  }
	  transducers[index].id = 0x016002cd;  // Bernard's Intuos 2 pen
	} else {
	  transducers[index].type = CURSOR;
	  transducers[index].id = 0xfe9ffd32;  // Invert Bernards's Intuos 2 Pen
	}
	queue_message(TOOL_IN, index);
      }
    
      transducers[index].location_x = ((uint16_t)msg[1] << 8) | msg[2];
      transducers[index].location_x = ((uint16_t)msg[3] << 8) | msg[4];

      switch (transducers[index].type) {
      case STYLUS_STANDARD:
	transducers[index].buttons = (msg[0] & 0x06) >> 1;
	// Drop through
      case STYLUS_STANDARD_INVERTED:
	transducers[index].touching = msg[0] & 0x01;

	transducers[index].pressure = msg[5];
	transducers[index].tilt_x = msg[6];
	transducers[index].tilt_y = msg[7];
	break;
      default:
	// Cursor
	// Ultrapad cursor only has 4 buttons, intuos has 5.
	transducers[index].buttons = msg[0] & 0x0f;
	transducers[index].touching = 1;
	break;
      }
      queue_message(TOOL_UPDATE, index);
      break;
    default:
      if (msg[0] & 0x10) {
	// Macro button click.  Need to decide how to handle this.
      } else {
	// Out of proximity.
	queue_message(TOOL_OUT, index);
	memset((void*)&(transducers[index]), 0, sizeof(transducer_t));
      }
      break;
    }
  }
}

void handle_calcomp_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
}

// Overall r0 message handler
void handle_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
  switch (tablet_family) {
  case INTUOS:
    handle_gd_r0_message(msg_length, msg);
    break;
  case ULTRAPAD:
    handle_ud_r0_message(msg_length, msg);
    break;
  case CALCOMP:
    handle_calcomp_r0_message(msg_length, msg);
  default:
    break;
  }
}
