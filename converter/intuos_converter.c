/* Nasty hacky intuos adb tablet -> usb intuos 3 converter
 * Copyright (c) 2013 Simon Stapleton, portions (c) 2008 PJRC.COM, LTD
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <ctype.h>
#include <util/delay.h>
#include <util/atomic.h>
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

// ADB / USB initialisation and polling loop
int main(void)
{
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
  adb_command_just_finished = 0;
  the_packet.address = 4;
  the_packet.command = ADB_COMMAND_TALK;
  the_packet.parameter = 1;
  the_packet.datalen = 0;

  initiateAdbTransfer(&the_packet, &adb_callback);
  //_delay_ms(1000);

  while(!adb_command_just_finished) {_delay_ms(8);};
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

  // Carry on with initialising the tablet.
  // Not sure if these steps are necessary
  // Listen R2 0x538c (sniffed from mac traffic by Bernard)
  adb_command_just_finished = 0;
  the_packet.command = ADB_COMMAND_LISTEN;
  the_packet.parameter = 2;
  the_packet.datalen = 2;
  the_packet.data[0] = 0x53;
  the_packet.data[1] = 0x8c;

  initiateAdbTransfer(&the_packet, &adb_callback);
  while(!adb_command_just_finished) {_delay_ms(8);}

  // Listen R2 0x308c (sniffed from mac traffic by Bernard)
  adb_command_just_finished = 0;
  the_packet.command = ADB_COMMAND_LISTEN;
  the_packet.parameter = 2;
  the_packet.datalen = 2;
  the_packet.data[0] = 0x30;
  the_packet.data[1] = 0x8c;

  initiateAdbTransfer(&the_packet, &adb_callback);
  while(!adb_command_just_finished) {_delay_ms(8);}

  // Set up for polling
  adb_command_just_finished = 0;
  the_packet.address = 4;
  the_packet.command = ADB_COMMAND_TALK;
  the_packet.parameter = 0;
  the_packet.datalen = 0;
  initiateAdbTransfer(&the_packet, &adb_callback);

  // The tablet is now up, we can start polling R0 for data
  uint8_t copied_adb[8];
  while (1) {
    if(adb_command_just_finished) {
      adb_command_just_finished = 0;
      if (the_packet.datalen > 0) {
	copied_adb[0] = the_packet.data[0];
	copied_adb[1] = the_packet.data[1];
	copied_adb[2] = the_packet.data[2];
	copied_adb[3] = the_packet.data[3];
	copied_adb[4] = the_packet.data[4];
	copied_adb[5] = the_packet.data[5];
	copied_adb[6] = the_packet.data[6];
	copied_adb[7] = the_packet.data[7];
	//	error_condition (the_packet.datalen);
	handle_r0_message(the_packet.datalen, copied_adb);
      }
      the_packet.address = 4;
      the_packet.command = ADB_COMMAND_TALK;
      the_packet.parameter = 0;
      the_packet.datalen = 0;
      initiateAdbTransfer(&the_packet, &adb_callback);
    }
  }
}




