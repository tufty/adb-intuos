#ifndef __USB_SIDE_PRIV_H__
#define __USB_SIDE_PRIV_H__

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
    uint16_t x;
    uint16_t y;
    union {
      struct {
	struct {
	  unsigned tilt_y:7;
	  unsigned tilt_x:7;
	  unsigned pressure:10;
	};
	struct {
	  unsigned y_lowest:1;
	  unsigned x_lowest:1;
	  unsigned distance_lowest:1;
	  unsigned distance:5;
	};
      } pen_payload;
      struct {
	struct {
	  unsigned unused_0:5;
	  unsigned rotation:11;
	};
	uint8_t unused_1;
	struct {
	  unsigned buttons:6;
	  unsigned z:8;
	};
      } mouse_4d_payload;
      struct {
	uint16_t unused_2;
	struct {
	  unsigned wheel_up:1;
	  unsigned wheel_down:1;
	  unsigned left_button:1;
	  unsigned middle_button:1;
	  unsigned right_button:1;
	  unsigned unused_3:3;
	};
	uint16_t unused_4;
      } mouse_2d_payload;
      struct {
	uint16_t unused_5;
	struct {
	  unsigned left_button:1;
	  unsigned middle_button:1;
	  unsigned right_button:1;
	  unsigned lower_right_button:1;
	  unsigned lower_left_button:1;
	  unsigned unused_6:3;
	};
	uint16_t unused_7;
      } lens_cursor_payload;
    };
  };
} wacom_report_t;

void populate_in_proximity(uint8_t index, wacom_report_t * packet);
void populate_out_of_proximity(uint8_t index, wacom_report_t * packet);
void populate_update(uint8_t index, wacom_report_t * packet);

#endif
