#ifndef __SHARED_STATE_H__
#define __SHARED_STATE_H__

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

#include "transducer.h"
#include "buttons.h"

typedef struct {
  const uint16_t product_id;
  const int16_t product_name[10];
} product_id_t; 

typedef struct {
  enum { INTUOS1, ULTRAPAD, CALCOMP } tablet_family;
  uint16_t max_x; 
  uint16_t max_y;
  uint16_t button_width;
  uint8_t n_buttons;
  button_t buttons[32];
} source_tablet_t;

typedef struct {
  enum { INTUOS2, INTUOS3 } tablet_family;
  product_id_t product_id;
  uint16_t max_x; 
  uint16_t max_y;
  uint16_t button_width;
  uint8_t n_buttons;
  button_t buttons[32];
} tablet_t;

extern transducer_t transducers[2];

extern source_tablet_t source_tablet;
extern tablet_t target_tablet; 

#endif
