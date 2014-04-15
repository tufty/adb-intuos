#ifndef __TRANSFORMS_H__
#define __TRANSFORMS_H__

#include <stdint.h>

#define TARGET_INTUOS_2

uint16_t location_to_location(uint16_t raw);
uint8_t tilt_to_tilt(uint8_t raw);
uint16_t pressure_to_pressure(uint16_t raw);
uint16_t rotation_to_rotation(uint16_t raw);
uint16_t z_to_z(uint16_t raw);

#endif
