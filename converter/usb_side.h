#ifndef __USB_SIDE_H__
#define __USB_SIDE_H__

// Copyright (c) 2013-2014 by Simon Stapleton (simon.stapleton@gmail.com) 
// Portions (c) 2010 Bernard Poulin (bernard-at-acm-dot-org) as part of the 
// Waxbee project
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

#include <stdint.h>
#include "shared_state.h"
#include "usb.h"

#define WACOM_INTUOS5_PEN_ENDPOINT 3
#define WACOM_INTUOS2_PEN_ENDPOINT 1

// Number of 5ms ticks before resending the last usb packet
#define USB_IDLE_TIME_LIMIT 8

#define TIMER0_PRESCALER_DIVIDER        1024
#define TIMER0_PRESCALER_SETTING        (BITV(CS10, 1) | BITV(CS11, 0) | BITV(CS12, 1))

void error_condition(uint8_t error);
void identify_product(uint16_t product_id);
void queue_message (message_type_t type, uint8_t transducer);

#endif
