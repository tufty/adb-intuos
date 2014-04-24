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

// Transform "raw" tabet data into a format acceptable to the Wacom driver
// Specific to source and target families 

#include "transforms.h"
#include "shared_state.h"

#ifdef TARGET_INTUOS_2

// Transform a location in source form to target form
// Effectively a null op for intuos 2 target
uint16_t x_to_x(uint16_t raw) {
  uint32_t transformed;
  switch (source_tablet.tablet_family) {
  case CALCOMP:
    // Rescale dpi to dpi
    transformed = (raw * target_tablet.max_y);
    transformed /= source_tablet.max_y;
    return transformed;
  default:
    return raw;
  }
}

uint16_t y_to_y(uint16_t raw) {
  if (target_tablet.tablet_family == INTUOS2) {
    uint32_t transformed;
    switch (source_tablet.tablet_family) {
    case ULTRAPAD: 
      // Rescale y to fit the active area
      transformed = target_tablet.max_y - target_tablet.menu_height;
      transformed *= raw;
      transformed /= source_tablet.max_y;
      return (uint16_t)transformed + target_tablet.menu_height;
    case INTUOS1:
      return raw;
    default:
      // Rescale dpi to dpi
      transformed = (raw * target_tablet.max_y);
      transformed /= source_tablet.max_y;
      return transformed;
    }
  } else {
    return raw;
  }
}

// Transform a z reading from source form to target form
// Z represents the 4d mouse wheel
// Intuos 1 tablet reports 0 <= x <= 3ff
// Driver expects 0 <= x <= 0xff + sign bit
// We get the sign bit with the button packet, pre-interleaved for the
// USB report so we only need to deal with scaling the raw value
uint16_t z_to_z(uint16_t raw) {
  // shift raw value 2 bits to the right
  return (raw & 0x3fc) >> 2;
}

// Transform a tilt reading in source form to target form
// Again, effectively a null op for intuos2, values are identical
uint8_t tilt_to_tilt(uint8_t raw) {
  return raw & 0x7f;
}

// Transform a pressure reading in source form to target form
// Null op for intuos 2
uint16_t pressure_to_pressure(uint16_t raw) {
  return raw & 0x3ff;
}

// Transform a rotation reading from source form to target form
// Intuos 1 tablet reports 0 <= x <= 0x7ff, with midpoint at 0x707
// Driver expects magnitude 0 <= x <= 0x3ff followed by a sign bit
// Rotation anticlockwise is +ve
uint16_t rotation_to_rotation(uint16_t raw, uint8_t sign) {
  return ((raw & 0x7ff) << 1) | sign;
}

#endif
