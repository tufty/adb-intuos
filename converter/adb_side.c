#include "adb_side.h"
#include "usb_side.h"
#include "led.h"
#include "shared_state.h"

#include <string.h>

// Parser for "talk register 1" messages
void handle_r1_message (uint8_t msg_length, volatile uint8_t * msg) {
  if (msg_length == 0)
    error_condition(10);
  if (msg_length < 8) {
    error_condition(msg_length);
  }

  uint16_t id_product = 0xdead;
  uint16_t max_x = ((uint16_t)(msg[2]) << 8) | (uint16_t)msg[3];
  uint16_t max_y = ((uint16_t)(msg[4]) << 8) | (uint16_t)msg[5];

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
    id_product = 0x4300;    // Intuos 2 9x12
    break;
  case 0x7710:
    if (max_y == max_x) {
      id_product = 0x4400;  // Intuos 2 12x12
    } else { 
      id_product = 0x4800;  // Intuos 2 12x18
    }
    break;
  default:
    // Error 1 - Failed to recognise tablet
    error_condition(9);
    break;
  }

  identify_product(id_product);
}

// Averaging of locations
void average_location(uint8_t average, uint16_t loc, uint16_t * p_loc, uint16_t * p_old_loc) {
  if (average) {
    *p_loc = (loc + * p_old_loc) >> 1;
    *p_old_loc = loc;
  } else {
    *p_loc = loc;
    *p_old_loc = loc;
  }
}

// Process a location delta.  This one's a bit hairy
void process_location_delta (uint8_t raw, uint8_t * shift, uint16_t * location, uint16_t * old_location) {
  uint8_t magnitude = raw & 0x0f;
  uint16_t delta = magnitude << *shift;
  uint16_t new_location;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x10) {
    new_location = *location - delta;
  } else {
    new_location = *location + delta;
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

  // there's an averaging step here, that the other delta handlers don't have
  average_location(1, new_location, location, old_location);
}

void process_tilt_delta (uint8_t raw, uint8_t * shift, uint16_t * value) {
  uint8_t magnitude = raw & 0x07;
  uint16_t delta = magnitude << *shift;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x08) {
    *value -= delta;
  } else {
    *value += delta;
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
}


void process_pressure_delta(uint8_t raw, uint8_t * shift, uint16_t * value) {
  uint8_t magnitude = raw & 0x07;

  if (magnitude == 0x07) magnitude = 0x40;

  uint16_t delta = magnitude << *shift;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x08) {
    *value -= delta;
  } else {
    *value += delta;
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
}

void process_rotation_delta (uint8_t raw, uint8_t * shift, uint16_t * value) {
  uint8_t magnitude = raw & 0x03;
  uint16_t delta = magnitude << *shift;
  int8_t new_shift = *shift;

  // Calculate new location
  if (raw & 0x04) {
    *value -= delta;
  } else {
    *value += delta;
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
}


void handle_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
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
	average_location (transducers[transducer].entered_proximity, 
			  ((uint16_t)(msg[index + 1]) << 8) | msg[index + 2],
			  &transducers[transducer].location_x,
			  &transducers[transducer].location_x_old);
	average_location (transducers[transducer].entered_proximity, 
			  ((uint16_t)(msg[index + 3]) << 8) | msg[index + 4],
			  &transducers[transducer].location_y,
			  &transducers[transducer].location_y_old);
	// Calculate rotation.  urrrgly.
	if (msg[index + 6] & 0x10) {
	  transducers[transducer].rotation = 0x707 - ((((uint16_t)(msg[index + 5]) << 3) | (msg[index + 6] >> 5)) & 0x3ff);
	} else {
	  transducers[transducer].rotation = (((uint16_t)(msg[index + 5]) << 3) | (msg[index + 6] >> 5)) & 0x3ff;
	}

	transducers[transducer].location_x_shift = 4;
	transducers[transducer].location_y_shift = 4;

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

	average_location (transducers[transducer].entered_proximity, 
			  ((uint16_t)(msg[index + 1]) << 8) | msg[index + 2],
			  &transducers[transducer].location_x,
			  &transducers[transducer].location_x_old);
	average_location (transducers[transducer].entered_proximity, 
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

      average_location (transducers[transducer].entered_proximity, 
			((uint16_t)(msg[index + 1]) << 8) | msg[index + 2],
			&transducers[transducer].location_x,
			&transducers[transducer].location_x_old);
      average_location (transducers[transducer].entered_proximity, 
			((uint16_t)(msg[index + 3]) << 8) | msg[index + 4],
			&transducers[transducer].location_y,
			&transducers[transducer].location_y_old);

      transducers[transducer].pressure = ((uint16_t)(msg[index + 5]) << 2) | (msg[index + 6] >> 6);
      transducers[transducer].tilt_x = ((msg[index + 6] << 1) | (msg[index + 7] >> 7)) & 0x7f;
      transducers[transducer].tilt_y = msg[index + 7] & 0x7f;
      transducers[transducer].buttons = msg[index] & 0x06;

      transducers[transducer].location_x_shift = 4;
      transducers[transducer].location_y_shift = 4;
      transducers[transducer].tilt_x_shift = 4;
      transducers[transducer].tilt_y_shift = 4;
      transducers[transducer].pressure_shift = 3;

      transducers[transducer].entered_proximity = 0;
      index += 8;
      break;
    case 0x90:       // 7 byte packet, tool type and tool serial number (initial proximity packet)
    case 0x80:
      // extract transducer specific data
      transducer = (msg[index] & 0x10) >> 4;
      transducers[transducer].state = 0xc2;
      transducers[transducer].type = ((uint16_t)msg[index + 1] << 8) | (msg[index + 2] >> 4);
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
			      &transducers[transducer].location_x_old);
      process_location_delta (((msg[index] & 0x01) << 4) | (msg[index + 1] >> 4),
			      &transducers[transducer].location_y_shift,
			      &transducers[transducer].location_y,
			      &transducers[transducer].location_y_old);

      // Now switch based on transducer type.
      switch (transducers[transducer].type & 0xff7) {
      case STYLUS_STANDARD:
	// this must be an stylus major packet (that's all we get)
	process_pressure_delta(msg[index + 1] & 0x0f, 
			       &transducers[transducer].pressure_shift,
			       &transducers[transducer].pressure);
	index += 2;
	if (index + 2 < msg_length) {
	  process_tilt_delta (msg[index + 2] >> 4,
			      &transducers[transducer].tilt_x_shift,
			      &transducers[transducer].tilt_x);
	  process_tilt_delta (msg[index + 2] & 0x0f,
			      &transducers[transducer].tilt_y_shift,
			      &transducers[transducer].tilt_y);
	  index += 1;
	} 
	break;
      case MOUSE_4D:
	// 4D mouse has 2 states
	if (transducers[transducer].state == 0xaa) {
	  process_rotation_delta((msg[index + 1] & 0x0e) >> 1,
				 &transducers[transducer].rotation_shift,
				 &transducers[transducer].rotation);
	  if (msg[index + 1] & 0x01) {
	    transducers[transducer].rotation = 0x707 - transducers[transducer].rotation;
	  }
	  transducers[transducer].state = 0xa8;
	} else /* a8 */ {
	  process_pressure_delta(msg[index + 1] & 0x0f,
				 &transducers[transducer].z_shift,
				 &transducers[transducer].z);

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
      LED_TOGGLE;
      queue_message(TOOL_UPDATE, transducer);
      LED_TOGGLE;
    } 
  }
}
