// Transform "raw" tabet data into a format acceptable to the Wacom driver
// Specific to source and target families 

#include "transforms.h"

#ifdef TARGET_INTUOS_2

// Transform a location in source form to target form
// Effectively a null op for intuos 2 target
uint16_t location_to_location(uint16_t raw) {
  return raw;
}

// Transform a tilt reading in source form to target form
// Again, effectively a null op for intuos2, values are identical
uint8_t tilt_to_tilt(uint8_t raw) {
  return (raw - 0x40) & 0x7f;
}

// Transform a pressure reading in source form to target form
// Null op for intuos 2
uint16_t pressure_to_pressure(uint16_t raw) {
  return raw & 0x3ff;
}

// Transform a rotation reading from source form to target form
// Intuos 1 tablet reports 0 <= x <= 0x7ff, with midpoint at 0x707
// Driver expects magnitude 0 <= x <= 0x3ff followed by a sign bit
// Rotation anticlockwise is -ve
uint16_t rotation_to_rotation(uint16_t raw, uint8_t sign) {
  return (raw & 0x7fe) | sign;
}

// Transform a z reading from source form to target form
// Z represents the 4d mouse wheel
// Intuos 1 tablet reports 0 <= x <= 3ff
// Driver expects 0 <= x <= 0xff + sign bit
// Similar approach to rotation, unsigned magnitude and separate
// sign bit.  The data itself is spread over the USB packet, but
// we'll interleave the same way as for rotation, let the USB stuffer
// deal with it. 
uint16_t z_to_z(uint16_t raw) {
  uint8_t sign = raw > 0x1ff ? 0 : 1;
  uint16_t magnitude = sign ? 0x1ff - raw : raw - 0x1ff;

  return (magnitude & 0x1fe) | sign;
}

#endif
