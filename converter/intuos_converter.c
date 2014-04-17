// Copyright (c) 2013-2014 by Simon Stapleton (simon.stapleton@gmail.com)
// Portions (c) 2008 PJRC.COM, Ltd
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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <ctype.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>
#include "avr_util.h"
#include "adb_codec.h"
#include "adb_side.h"
#include "usb_side.h"
#include "led.h"
#include "debug.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

// Nasty globals
volatile uint8_t adb_command_just_finished = 0;
volatile AdbPacket the_packet;


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

// ADB / USB initialisation and polling loop
int main(void)
{
  uint8_t adb_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  // set for 16 MHz clock, and turn on the LED
  CPU_PRESCALE(0);
  LED_CONFIG;
  LED_OFF;

  sei();

  // Do things the other way around compared to adbterm
  // Initialise adb, get the tablet details, and then fire up USB
  // This will hopefully enable us to identify as the "correct" 
  // tablet type "automagically"
  _delay_ms(1000);  // Let the tablet power up

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    adb_init();
  }

  // First action : talk R3
  do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_3, 0, adb_data);

  // Determine tablet family based on handler ID
  switch (the_packet.data[1]) {
  case 0x3a:
    // Ultrapad.  Switch to handler 68 (stop behaving like a bloody mouse)
    tablet_family = ULTRAPAD;
    adb_data[0] = the_packet.data[0] | 1; adb_data[1] = 0x68;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_3, 2, adb_data);
    adb_data[0] &= 0xf8;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_1, 2, adb_data);    
    break;
  case 0x6a:
    tablet_family = INTUOS;
    adb_data[0] = 0x53; adb_data[1] = 0x8c;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_2, 2, adb_data);
    adb_data[1] = 0x30;
    do_adb_command(ADB_COMMAND_LISTEN, ADB_REGISTER_2, 2, adb_data);
    break;
  default:
    break;
  }
 
  // Now get the tablet details
  do_adb_command(ADB_COMMAND_TALK, ADB_REGISTER_1, 0, adb_data);

  // And populate the USB configurator
  handle_r1_message(the_packet.datalen, the_packet.data);

  // At this point, we should be able to bring up USB, and get the correct configuration
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    usb_init();
  }

  // Wait for USB configuration to be done
  while (!usb_configured());

  // Wait for a second to let the host OS catch up
  _delay_ms(1000);

  // Send a couple of tool out messages to tell the driver what's in proximity
  queue_message(TOOL_OUT, 0);
  queue_message(TOOL_OUT, 1);

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




