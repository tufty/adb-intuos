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

  wakeup_tablet();


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

  poll_tablet();
}




