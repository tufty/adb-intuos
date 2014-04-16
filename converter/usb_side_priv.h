#ifndef __USB_SIDE_PRIV_H__
#define __USB_SIDE_PRIV_H__

// Copyright (c) 2013-2014 by Simon Stapleton (simon.stapleton@gmailcom) 
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
#include <string.h>

typedef struct {
  const uint16_t product_id;
  const int16_t product_name[10];
} product_id_t; 

extern const product_id_t product_ids[];
extern const uint8_t n_product_ids;

typedef struct {
  uint8_t length;
  uint8_t type;
  int16_t string[];
} usb_string_t;

// Need this to tell the HID stack what we are, and let the Wacom driver hook up
extern uint8_t device_descriptor[18];
const extern uint8_t config_descriptor[34];
const extern uint8_t hid_report_descriptor[154];

const extern usb_string_t string0;
const extern usb_string_t string1;
extern usb_string_t string2;
const extern usb_string_t empty_string;

typedef union {
  uint8_t bytes[10];
  struct {
    uint8_t hid_identifier;
    struct
    {
      unsigned tool_index:1;
      unsigned bit1:1;
      unsigned bit2:1;
      unsigned bit3:1; 
      unsigned bit4:1;
      unsigned proximity:1;
      unsigned bit6:1;
      unsigned bit7:1;
    };
    uint8_t x_hi;
    uint8_t x_lo;
    uint8_t y_hi;
    uint8_t y_lo;
    uint8_t payload[3];
    struct {
      unsigned y_lowest:1;
      unsigned x_lowest:1;
      unsigned distance_lowest:1;
      unsigned distance:5;
    };
  };
} wacom_report_t;

void populate_in_proximity(uint8_t index, wacom_report_t * packet);
void populate_out_of_proximity(uint8_t index, wacom_report_t * packet);
void populate_update(uint8_t index, wacom_report_t * packet);

#endif
