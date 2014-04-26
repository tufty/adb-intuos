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

#include <util/atomic.h>
#include <avr/pgmspace.h>
#include "adb_side.h"
#include "adb_codec.h"
#include "usb_side.h"
#include "buttons.h"
#include "led.h"
#include "shared_state.h"
#include "debug.h"

#include <string.h>

// Nasty globals
volatile uint8_t adb_command_just_finished = 0;
volatile AdbPacket the_packet;
uint8_t adb_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

transducer_t transducers[2];
source_tablet_t source_tablet;
tablet_t target_tablet;

const source_tablet_t ultrapad_a6 PROGMEM = {
  ULTRAPAD,
  0x3200, 0x2580, 880, 0, {}
};
const source_tablet_t ultrapad_a5 PROGMEM = {
  ULTRAPAD,
  0x5000, 0x3c00, 880, 18,
  {
    {BTN_F1, 0}, {BTN_F2, 0}, {BTN_F3, 0}, {BTN_F4, 0}, {BTN_F5, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_UNDO, 0}, {BTN_DEL, 0},
    {BTN_NEW, 0}, {BTN_OPEN, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0},
    {BTN_PEN, 0}, {BTN_MOUSE, 0}, 
    {BTN_SOFT, 0}, {BTN_FIRM, 0}
  }
};
const source_tablet_t ultrapad_a4 PROGMEM = {
  ULTRAPAD,
  0x7710, 0x5dfc, 1100, 23,
  {
    {BTN_SETUP, 0}, 
    {BTN_F1, 0}, {BTN_F2, 0}, {BTN_F3, 0}, {BTN_F4, 0}, {BTN_F5, 0},
    {BTN_F6, 0}, {BTN_F7, 0}, {BTN_F8, 0}, {BTN_F9, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_UNDO, 0}, {BTN_DEL, 0},
    {BTN_NEW, 0}, {BTN_OPEN, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0},
    {BTN_PEN, 0}, {BTN_MOUSE, 0}, 
    {BTN_SOFT, 0}, {BTN_FIRM, 0}
  }
};
const source_tablet_t ultrapad_a4plus PROGMEM = {
  ULTRAPAD,
  0x7710, 0x7710, 1100, 23,
  {
    {BTN_SETUP, 0}, 
    {BTN_F1, 0}, {BTN_F2, 0}, {BTN_F3, 0}, {BTN_F4, 0}, {BTN_F5, 0},
    {BTN_F6, 0}, {BTN_F7, 0}, {BTN_F8, 0}, {BTN_F9, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_UNDO, 0}, {BTN_DEL, 0},
    {BTN_NEW, 0}, {BTN_OPEN, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0},
    {BTN_PEN, 0}, {BTN_MOUSE, 0}, 
    {BTN_SOFT, 0}, {BTN_FIRM, 0}
  }
};

const source_tablet_t ultrapad_a3 PROGMEM = {
  ULTRAPAD,
  0xb298, 0x7710, 1100, 34,
  {
    {BTN_SETUP, 0}, 
    {BTN_F1, 0}, {BTN_F2, 0}, {BTN_F3, 0}, {BTN_F4, 0}, {BTN_F5, 0},
    {BTN_F6, 0}, {BTN_F7, 0}, {BTN_F8, 0}, {BTN_F9, 0}, {BTN_F10, 0},
    {BTN_F11, 0}, {BTN_F12, 0}, {BTN_F13, 0}, {BTN_F14, 0}, {BTN_F15, 0},
    {BTN_F16, 0}, {BTN_F17, 0}, {BTN_F18, 0}, {BTN_F19, 0}, {BTN_F20, 0}, {BTN_F21, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_UNDO, 0}, {BTN_DEL, 0},
    {BTN_NEW, 0}, {BTN_OPEN, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0},
    {BTN_PEN, 0}, {BTN_MOUSE, 0}, 
    {BTN_SOFT, 0}, {BTN_FIRM, 0}
  }
};


const source_tablet_t intuos1_a6 PROGMEM = {  // Intuos 1 A6
  INTUOS1, 
  0x319c, 0x2968, 880, 11,
  {
    {BTN_NEW,0}, {BTN_OPEN,0}, {BTN_CLOSE, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0}, {BTN_EXIT, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_PEN, 0}, {BTN_MOUSE, 0}
  }
};
const source_tablet_t intuos1_a5 PROGMEM = {  // Intuos 1 A5
  INTUOS1, 
  0x4f60, 0x3f70, 880, 18,
  {
    {BTN_NEW, 320}, {BTN_OPEN, 1380}, {BTN_CLOSE, 2420}, {BTN_SAVE, 3490}, {BTN_PRINT, 4550}, {BTN_EXIT, 5520},
    {BTN_CUT, 6990}, {BTN_COPY, 8070}, {BTN_PASTE, 9150}, {BTN_UNDO, 10130}, {BTN_DEL, 11210},
    {BTN_F12, 12650}, {BTN_F13, 13700}, 
    {BTN_PEN, 13700 + 1380}, {BTN_MOUSE, 13700 + 2480},
    {BTN_SOFT, 13700 + 3820}, {BTN_MED, 13700 + 4890}, {BTN_FIRM, 13700 + 5940}
  }
};
const source_tablet_t intuos1_a4 PROGMEM = {  // Intuos 1 A4
  INTUOS1, 
  0x7710, 0x5dfc, 1100, 22,
  {
    {BTN_NEW, 300}, {BTN_OPEN, 1650}, {BTN_CLOSE, 3000}, {BTN_SAVE, 4300}, {BTN_PRINT, 5600}, {BTN_EXIT, 6900},
    {BTN_CUT, 8600}, {BTN_COPY, 10000}, {BTN_PASTE, 11300}, {BTN_UNDO, 12600}, {BTN_DEL, 13900},
    {BTN_F12, 15600}, {BTN_F13, 16900}, {BTN_F14, 18300}, {BTN_F15, 19600}, {BTN_F16, 20900}, 
    {BTN_PEN, 22600}, {BTN_MOUSE, 23900}, {BTN_QUICKPOINT, 25300},
    {BTN_SOFT, 26900}, {BTN_MED, 28200}, {BTN_FIRM, 29500}
  }
};
const source_tablet_t intuos1_a4plus PROGMEM = {  // Intuos 1 A4 Plus
  INTUOS1, 
  0x7710, 0x7bc0, 1100, 22,
  {
    {BTN_NEW, 300}, {BTN_OPEN, 1650}, {BTN_CLOSE, 3000}, {BTN_SAVE, 4300}, {BTN_PRINT, 5600}, {BTN_EXIT, 6900},
    {BTN_CUT, 8600}, {BTN_COPY, 10000}, {BTN_PASTE, 11300}, {BTN_UNDO, 12600}, {BTN_DEL, 13900},
    {BTN_F12, 15600}, {BTN_F13, 16900}, {BTN_F14, 18300}, {BTN_F15, 19600}, {BTN_F16, 20900}, 
    {BTN_PEN, 22600}, {BTN_MOUSE, 23900}, {BTN_QUICKPOINT, 25300},
    {BTN_SOFT, 26900}, {BTN_MED, 28200}, {BTN_FIRM, 29500}
  }
};
const source_tablet_t intuos1_a3 PROGMEM = { // Intuos 1 A3
  INTUOS1, 
  0xb298, 0x7bc0, 1100, 32,
  {
    {BTN_NEW, 0}, {BTN_OPEN, 0}, {BTN_CLOSE, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0}, {BTN_EXIT, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_UNDO, 0}, {BTN_DEL, 0},
    {BTN_F12, 0}, {BTN_F13, 0}, {BTN_F14, 0}, {BTN_F15, 0}, {BTN_F16, 0}, {BTN_F17, 0}, {BTN_F18, 0}, {BTN_F19, 0},
    {BTN_F20, 0}, {BTN_F21, 0}, {BTN_F22, 0}, {BTN_F23, 0}, {BTN_F24, 0}, {BTN_F25, 0}, {BTN_F26, 0}, {BTN_F27, 0},
    {BTN_PEN, 0}, {BTN_MOUSE, 0},
    {BTN_SOFT, 0}, {BTN_MED, 0}, {BTN_FIRM, 0}
  }
};

// Intuos 2 target tablet definitions
const tablet_t intuos2_a6 PROGMEM = {
  INTUOS2, { 0x4100, L"XD-0405-U" },
  0x319c, 0x2968, 880, 11,
  {
    {BTN_NEW,0}, {BTN_OPEN,0}, {BTN_CLOSE, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0}, {BTN_EXIT, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_PEN, 0}, {BTN_MOUSE, 0}
  }  
};

const tablet_t intuos2_a5 PROGMEM = {
  INTUOS2, { 0x4200, L"XD-0608-U" },
  0x4f60, 0x3f70, 880, 18,
  {
    {BTN_NEW, 320}, {BTN_OPEN, 1380}, {BTN_CLOSE, 2420}, {BTN_SAVE, 3490}, {BTN_PRINT, 4550}, {BTN_EXIT, 5520},
    {BTN_CUT, 6990}, {BTN_COPY, 8070}, {BTN_PASTE, 9150}, {BTN_UNDO, 10130}, {BTN_DEL, 11210},
    {BTN_F12, 12650}, {BTN_F13, 13700}, 
    {BTN_PEN, 13700 + 1380}, {BTN_MOUSE, 13700 + 2480},
    {BTN_SOFT, 13700 + 3820}, {BTN_MED, 13700 + 4890}, {BTN_FIRM, 13700 + 5940}
  }
};
const tablet_t intuos2_a4 PROGMEM = {
  INTUOS2, { 0x4300, L"XD-0912-U" },
  0x7710, 0x5dfc, 1100, 22,
  {
    {BTN_NEW, 300}, {BTN_OPEN, 1650}, {BTN_CLOSE, 3000}, {BTN_SAVE, 4300}, {BTN_PRINT, 5600}, {BTN_EXIT, 6900},
    {BTN_CUT, 8600}, {BTN_COPY, 10000}, {BTN_PASTE, 11300}, {BTN_UNDO, 12600}, {BTN_DEL, 13900},
    {BTN_F12, 15600}, {BTN_F13, 16900}, {BTN_F14, 18300}, {BTN_F15, 19600}, {BTN_F16, 20900}, 
    {BTN_PEN, 22600}, {BTN_MOUSE, 23900}, {BTN_QUICKPOINT, 25300},
    {BTN_SOFT, 26900}, {BTN_MED, 28200}, {BTN_FIRM, 29500}
  }
};
const tablet_t intuos2_a4plus PROGMEM = {
  INTUOS2, { 0x4400, L"XD-1212-U" },
  0x7710, 0x7bc0, 1100, 22,
  {
    {BTN_NEW, 300}, {BTN_OPEN, 1650}, {BTN_CLOSE, 3000}, {BTN_SAVE, 4300}, {BTN_PRINT, 5600}, {BTN_EXIT, 6900},
    {BTN_CUT, 8600}, {BTN_COPY, 10000}, {BTN_PASTE, 11300}, {BTN_UNDO, 12600}, {BTN_DEL, 13900},
    {BTN_F12, 15600}, {BTN_F13, 16900}, {BTN_F14, 18300}, {BTN_F15, 19600}, {BTN_F16, 20900}, 
    {BTN_PEN, 22600}, {BTN_MOUSE, 23900}, {BTN_QUICKPOINT, 25300},
    {BTN_SOFT, 26900}, {BTN_MED, 28200}, {BTN_FIRM, 29500}
  }
};
const tablet_t intuos2_a3 PROGMEM = {
  INTUOS2, { 0x4800, L"XD-1218-U" },
  0xb298, 0x7bc0, 1100, 32,
  {
    {BTN_NEW, 0}, {BTN_OPEN, 0}, {BTN_CLOSE, 0}, {BTN_SAVE, 0}, {BTN_PRINT, 0}, {BTN_EXIT, 0},
    {BTN_CUT, 0}, {BTN_COPY, 0}, {BTN_PASTE, 0}, {BTN_UNDO, 0}, {BTN_DEL, 0},
    {BTN_F12, 0}, {BTN_F13, 0}, {BTN_F14, 0}, {BTN_F15, 0}, {BTN_F16, 0}, {BTN_F17, 0}, {BTN_F18, 0}, {BTN_F19, 0},
    {BTN_F20, 0}, {BTN_F21, 0}, {BTN_F22, 0}, {BTN_F23, 0}, {BTN_F24, 0}, {BTN_F25, 0}, {BTN_F26, 0}, {BTN_F27, 0},
    {BTN_PEN, 0}, {BTN_MOUSE, 0},
    {BTN_SOFT, 0}, {BTN_MED, 0}, {BTN_FIRM, 0}
  }
};  

void adb_callback(uint8_t error_code) {
  adb_command_just_finished = 1;
  if (error_code != 0) {
    the_packet.datalen = 0;
    error_condition(error_code);
  }
}

void do_adb_command(uint8_t command, uint8_t parameter, uint8_t length, uint8_t * data) {
  adb_command_just_finished = 0;
  the_packet.address = 4;
  the_packet.command = command;
  the_packet.parameter = parameter;
  the_packet.datalen = length;

  if (length) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      memcpy((void*)the_packet.data, data, length);
    }
  }

  initiateAdbTransfer(&the_packet, &adb_callback);
  
  // Spin!
  while (!adb_command_just_finished);
}

// Parser for "talk register 1" messages
void handle_wacom_r1_message (uint8_t msg_length, volatile uint8_t * msg) {
  const tablet_t * target = 0; 
  const source_tablet_t * source = 0;

  if (msg_length == 0)
    error_condition(10);
  if (msg_length < 8) {
    error_condition(msg_length);
  }

  source_tablet.max_x = ((uint16_t)(msg[2]) << 8) | (uint16_t)msg[3];
  source_tablet.max_y = ((uint16_t)(msg[4]) << 8) | (uint16_t)msg[5];

  switch (source_tablet.tablet_family) {
  case ULTRAPAD:
    if (source_tablet.max_x == pgm_read_word(&ultrapad_a6.max_x)) {
      source = &ultrapad_a6;
      target = &intuos2_a6;
    } else if (source_tablet.max_x == pgm_read_word(&ultrapad_a5.max_x)) {
      source = &ultrapad_a5;
      target = &intuos2_a5;      
    } else if (source_tablet.max_x == pgm_read_word(&ultrapad_a4.max_x)) {
      if (source_tablet.max_y == pgm_read_word(&ultrapad_a4.max_y)) {
	source = &ultrapad_a4;
	target = &intuos2_a4;      
      } else {
	source = &ultrapad_a4plus;
	target = &intuos2_a4plus;      
      }
    } else {
      source = &ultrapad_a3;
      target = &intuos2_a3;
    }
    break;
  case INTUOS1:
    if (source_tablet.max_x == pgm_read_word(&intuos1_a6.max_x)) {
      source = &intuos1_a6;
      target = &intuos2_a6;
    } else if (source_tablet.max_x == pgm_read_word(&intuos1_a5.max_x)) {
      source = &intuos1_a5;
      target = &intuos2_a5;      
    } else if (source_tablet.max_x == pgm_read_word(&intuos1_a4.max_x)) {
      if (source_tablet.max_y == pgm_read_word(&intuos1_a4.max_y)) {
	source = &intuos1_a4;
	target = &intuos2_a4;      
      } else {
	source = &intuos1_a4plus;
	target = &intuos2_a4plus;      
      }
    } else {
      source = &intuos1_a3;
      target = &intuos2_a3;
    }
    break;
  default:
    break;
  }

  memcpy_P(&source_tablet, source, sizeof(source_tablet_t));
  memcpy_P(&target_tablet, target, sizeof(tablet_t));
  
  identify_product();
}

void handle_calcomp_r1_message(uint8_t msg_length, volatile uint8_t * msg) {
  switch (msg[1]) {
  case 0:
    //    source_tablet.tablet_format = target_tablet.tablet_format = A6;
    // Calcomp 4x5
    //id_product = 0x4100;    // Intuos 2 4x5
    break;
  case 1:
    //    source_tablet.tablet_format = target_tablet.tablet_format = A5;
    // Calcomp 6x9
    //id_product = 0x4200;    // Intuos 2 6x8
    break;
  case 2:
    //    source_tablet.tablet_format = target_tablet.tablet_format = A4PLUS;
    // Calcomp 12x12
    //id_product = 0x4400;    // Intuos 2 12x12
    break;
  case 3:
    //    source_tablet.tablet_format = target_tablet.tablet_format = A3;
    // Calcomp 12x18
    //id_product = 0x4800;    // Intuos 2 12x18
    break;
  }
 
  source_tablet.max_x = (msg[3] >> 1) * 1000;
  source_tablet.max_y = (msg[4] >> 1) * 1000;
  
}

void handle_r1_message(uint8_t msg_length, volatile uint8_t * msg) { 
  switch (source_tablet.tablet_family) {
  case INTUOS1:
  case ULTRAPAD:
    handle_wacom_r1_message(msg_length, msg);
    break;
  case CALCOMP:
    handle_calcomp_r1_message(msg_length, msg);
    break;
  default:
    break;
  }
}

void wakeup_tablet() {
  // First action : talk R3
  do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_3, 0, adb_data);

  // Determine tablet family based on handler ID
  switch (the_packet.data[1]) {
  case 0x3a:
    // Ultrapad.  Switch to handler 68 (stop behaving like a bloody mouse)
    source_tablet.tablet_family = ULTRAPAD;
    adb_data[0] = the_packet.data[0] | 1; adb_data[1] = 0x68;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_3, 2, adb_data);
    adb_data[0] &= 0xf8;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_1, 2, adb_data);
    // Now get the tablet details
    do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_1, 0, adb_data);
    break;
  case 0x40:
    // Calcomp handler.  Need to tell the tablet stop behaving like a mouse
    // This hapens with either a talk register 1 or a talk register 2, dependent 
    // on firmware date.  No, we don't know the tablet's firmware date.
    source_tablet.tablet_family = CALCOMP;
    do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_1, 0, adb_data);
    if (the_packet.datalen == 0) {
      do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_2, 0, adb_data);
    }
    break;
  case 0x6a:
    // Intuos
    source_tablet.tablet_family = INTUOS1;
    adb_data[0] = 0x53; adb_data[1] = 0x8c;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_2, 2, adb_data);
    adb_data[1] = 0x30;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_2, 2, adb_data);
    // Now get the tablet details
    do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_1, 0, adb_data);
    break;
  default:
    break;
  }
 
  // And populate the USB configurator
  handle_r1_message(the_packet.datalen, the_packet.data);
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
			      source_tablet.max_x);
      process_location_delta (((msg[index] & 0x01) << 4) | (msg[index + 1] >> 4),
			      &transducers[transducer].location_y_shift,
			      &transducers[transducer].location_y,
			      &transducers[transducer].location_y_old,
			      source_tablet.max_y);

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
  } else if (msg_length == 8) {
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
      transducers[index].location_y = ((uint16_t)msg[3] << 8) | msg[4];

      switch (transducers[index].type) {
      case STYLUS_STANDARD:
	transducers[index].buttons = (msg[0] & 0x06) >> 1;
	// Drop through
      case STYLUS_STANDARD_INVERTED:
	transducers[index].touching = msg[0] & 0x01;

	transducers[index].pressure = msg[5] ^ 0x80;
	transducers[index].tilt_x = msg[6] - 0x40;
	transducers[index].tilt_y = msg[7] - 0x40;
	break;
      default:
	// Cursor
	// Ultrapad cursor only has 4 buttons, intuos has 5.
	// Ultrapad numbers buttons differently to intuos, not one bit per button.
	// This breaks chording.
	switch (msg[0] & 0x0f) {
	case 1: transducers[index].buttons = 2; break;
	case 2: transducers[index].buttons = 1; break;
	case 3: transducers[index].buttons = 8; break;
	case 4: transducers[index].buttons = 4; break;
	default: transducers[index].buttons = 0; break;
	}

	transducers[index].touching = 1;
	break;
      }
      queue_message(TOOL_UPDATE, index);
      break;
    default:
      if (msg[0] & 0x10) {
	uint8_t tool_button = msg[0] & 0x0f;
	// Macro button click.
	// Synthesize a tool in proximity message
	if (msg[0] & 0x20) {
	  transducers[index].type = STYLUS_STANDARD;
	  transducers[index].id = 0x016002cd;  // Bernard's Intuos 2 pen
	  tool_button >>= 1;
	} else {
	  transducers[index].type = CURSOR;
	  transducers[index].id = 0xfe9ffd32;  // Invert Bernards's Intuos 2 Pen
	}
	queue_message(TOOL_IN, index);
	// Then click and back out
	synthesize_button(index, source_tablet.buttons[msg[2]].button, tool_button);
	// No need to queue a tool out, we get one anyway
      } else {
	// Out of proximity.
	queue_message(TOOL_OUT, index);
	memset((void*)&(transducers[index]), 0, sizeof(transducer_t));
      }
      break;
    }
  } else {
    error_condition(0xff);
  }
}

void handle_calcomp_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
}

// Overall r0 message handler
void handle_r0_message(uint8_t msg_length, volatile uint8_t * msg) {
  switch (source_tablet.tablet_family) {
  case INTUOS1:
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

void poll_tablet() {
  // The tablet is now up, we can start polling R0 for data
  while (1) {
    do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_0, 0, adb_data);
    if (the_packet.datalen > 0) {
      adb_data[0] = the_packet.data[0];
      adb_data[1] = the_packet.data[1];
      adb_data[2] = the_packet.data[2];
      adb_data[3] = the_packet.data[3];
      adb_data[4] = the_packet.data[4];
      adb_data[5] = the_packet.data[5];
      adb_data[6] = the_packet.data[6];
      adb_data[7] = the_packet.data[7];
      handle_r0_message(the_packet.datalen, adb_data);
    }
  }
}
