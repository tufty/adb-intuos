#ifndef __TABLET_H__
#define __TABLET_H__

#include <stdint.h>

typedef enum {
  GD_OTHER,
  GD_0405_A,
  GD_0608_A,
  GD_0912_A,
  GD_1212_A
} gd_tablet_type_t;

struct {
  gd_tablet_type_t id;
  uint16_t max_x;
  uint16_t max_y;
} tablet;

#endif
