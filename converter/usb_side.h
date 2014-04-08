#ifndef __USB_SIDE_H__
#define __USB_SIDE_H__

#include <stdint.h>
#include "shared_state.h"
#include "usb.h"

#define WACOM_INTUOS5_PEN_ENDPOINT 3
#define WACOM_INTUOS2_PEN_ENDPOINT 1

void error_condition(uint8_t error);
void identify_product(uint16_t product_id);
void queue_message (message_type_t type, uint8_t transducer);

#endif
