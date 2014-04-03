#ifndef __TRANSDUCER_H__
#define __TRANSDUCER_H__

#include <stdint.h>

typedef enum message_type {
  TOOL_IN,
  TOOL_OUT,
  TOOL_UPDATE
} message_type_t;

typedef enum gd_tool_type {
  TOOL_NONE = 0,
  STYLUS_STANDARD = 0x822,
  STYLUS_INKING = 0x812,
  STYLUS_STROKE = 0x832,
  STYLUS_GRIP = 0x842,
  STYLUS_AIRBRUSH = 0x912,
  STYLUS_STANDARD_INVERTED = 0x822 | 0x008,
  STYLUS_INKING_INVERTED = 0x812 | 0x008,
  STYLUS_STROKE_INVERTED = 0x832 | 0x008,
  STYLUS_GRIP_INVERTED = 0x842 | 0x008,
  STYLUS_AIRBRUSH_INVERTED = 0x912 | 0x008,
  MOUSE_2D = 0x007,
  MOUSE_4D = 0x094,
  CURSOR = 0x096
} gd_tool_type_t;

typedef struct transducer {
  uint8_t state;

  uint32_t type;
  uint32_t id;
  uint16_t location_x;
  uint16_t location_x_old;
  uint16_t location_y;
  uint16_t location_y_old;
  uint16_t tilt_x;
  uint16_t tilt_y;
  uint16_t pressure;
  uint16_t z;
  uint16_t barrel_pressure;
  uint16_t rotation;
  uint16_t buttons;
  
  uint8_t entered_proximity;
  uint8_t touching;

  uint8_t location_x_shift;
  uint8_t location_y_shift;
  uint8_t tilt_x_shift;
  uint8_t tilt_y_shift;
  uint8_t pressure_shift;
  uint8_t z_shift;
  uint8_t barrel_pressure_shift;
  uint8_t rotation_shift;
} transducer_t;

#endif
