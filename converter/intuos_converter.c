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
#include <stdint.h>
#include <ctype.h>
#include <util/delay.h>
#include "avr_util.h"
#include "adb.h"
#include "adb_side.h"
#include "usb_side.h"
#include "led.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

// Nasty globals
uint8_t adb_command_just_finished = 0;
uint8_t adb_command_queued = 0;
AdbPacket the_packet;

void error_condition(uint8_t code)
{
  LED_OFF;

  while (1)
    {
      _delay_ms(2500);

      for(uint8_t i = code;i>0;i--)
	{
	  LED_ON;
	  _delay_ms(700);
	  LED_OFF;
	  _delay_ms(700);
	}
    }
}

void adb_callback(uint8_t errorCode) {
  LED_TOGGLE;
  adb_command_just_finished = 1;
  adb_command_queued = 0;
}

// ADB / USB initialisation and polling loop
int main(void)
{
  // set for 16 MHz clock, and turn on the LED
  CPU_PRESCALE(0);
  LED_CONFIG;
  LED_OFF;

  // Do things the other way around compared to adbterm
  // Initialise adb, get the tablet details, and then fire up USB
  // This will hopefully enable us to identify as the "correct" 
  // tablet type "automagically"
  _delay_ms(300);  // Let the tablet power up
  adb_init();

  LED_TOGGLE;

  // First action : talk R1, get tablet type
  adb_command_just_finished = 0;
  the_packet.address = 4;
  the_packet.command = ADB_COMMAND_TALK;
  the_packet.parameter = 1;
  the_packet.datalen = 0;

  initiateAdbTransfer(&the_packet, &adb_callback);
  //_delay_ms(1000);

  while(!adb_command_just_finished);
  handle_r1_message(the_packet.data);

  // At this point, we should be able to bring up USB, and get the correct configuration
  usb_init();
  while (!usb_configured());

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
  while(!adb_command_just_finished);

  LED_TOGGLE;
  
  // Listen R2 0x308c (sniffed from mac traffic by Bernard)
  adb_command_just_finished = 0;
  the_packet.command = ADB_COMMAND_LISTEN;
  the_packet.parameter = 2;
  the_packet.datalen = 2;
  the_packet.data[0] = 0x30;
  the_packet.data[1] = 0x8c;

  initiateAdbTransfer(&the_packet, &adb_callback);
  while(!adb_command_just_finished);

  LED_TOGGLE;

  // Set up for polling
  adb_command_just_finished = 0;
  the_packet.command = ADB_COMMAND_TALK;
  the_packet.parameter = 0;
  the_packet.datalen = 0;

  // The tablet is now up, we can start polling R0 for data
  while (1) {
    if (adb_command_queued) {
      _delay_ms(8);
    } else {
      if (adb_command_just_finished) {
	LED_TOGGLE;
	adb_command_just_finished = 0;
	if (the_packet.datalen > 0) {
	  handle_r0_message(the_packet.data);
	}
      } 
      adb_command_queued = 1;
      the_packet.datalen = 0;
      the_packet.data[0] = 0;
      the_packet.data[1] = 0;
      the_packet.data[2] = 0;
      the_packet.data[3] = 0;
      the_packet.data[4] = 0;
      the_packet.data[5] = 0;
      the_packet.data[6] = 0;
      the_packet.data[7] = 0;
      initiateAdbTransfer(&the_packet, &adb_callback);
    }
  }
}




