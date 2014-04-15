#include "usb_side.h"
#include "usb_side_priv.h"
#include "avr_util.h"
#include "transforms.h"
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "led.h"
#include "debug.h"

const product_id_t product_ids[] = {
  {0x4100, L"XD-0405-U"},
  {0x4200, L"XD-0608-U"},
  {0x4300, L"XD-0912-U"},
  {0x4400, L"XD-1212-U"},
  {0x4800, L"XD-1218-U"}
};

// Number of products we fake
const uint8_t n_product_ids = 5;

// Tablet device descriptor
// Pre-set up with the stuff we already know
uint8_t device_descriptor[18] = {
  0x12,		// bLength
  0x1,		// bDescriptorType
  0x0,0x2,	// bcdUSB - the Graphire3 is set at 1.1 (0x110) -- hope that is not a problem
  0x0,		// bDeviceClass
  0x0,		// bDeviceSubClass
  0x0,		// bDeviceProtocol
  0x8,		// bMaxPacketSize0 for endpoint 0
  0x6a,0x5,	// vendor id - WACOM Co., Ltd.
  0x00,0x0,     // product id - Set up on the fly
  0x26,0x1,	// bcdDevice - "version number" of the Wacom XD-...-U
  0x1,		// iManufacturer
  0x2,		// iProduct
  0x0,		// iSerialNumber
  0x1		// bNumConfigurations
};

// Config and HID report descriptors
// Ripped direct from Bernard's Intuos 2 descriptors.
// These are static
const uint8_t config_descriptor[34] = {
  0x9,		// bLength - USB spec 9.6.3, page 264-266, Table 9-10
  0x2,		// bDescriptorType;
  0x22,0x0,     // wTotalLength (9+9+9+7)
  0x1,		// bNumInterfaces
  0x1,		// bConfigurationValue
  0x0,		// iConfiguration
  0x80,		// bmAttributes
  0x4b,		// bMaxPower (mA/2) 150mA

  0x9,		// bLength - interface descriptor, USB spec 9.6.5, page 267-269, Table 9-12
  0x4,		// bDescriptorType
  0x0,		// bInterfaceNumber
  0x0,		// bAlternateSetting
  0x1,		// bNumEndpoints
  0x3,		// bInterfaceClass (0x03 = HID)
  0x1,		// bInterfaceSubClass (0x01 = Boot)
  0x1,		// bInterfaceProtocol (0x02 = Mouse)
  0x0,		// iInterface

  0x9,		// bLength - HID interface descriptor, HID 1.11 spec, section 6.2.1
  0x21,		// bDescriptorType
  0x0,0x1,      // bcdHID - Intuos2 is using HID version 1.00 (instead of 1.11)
  0x0,		// bCountryCode
  0x1,		// bNumDescriptors
  0x22,		// bDescriptorType
  0x9a,0x0,     // wDescriptorLength  HIDREPORTDESC Length (Graphire3 is 0x0062)

  0x7,		// bLength - endpoint descriptor, USB spec 9.6.6, page 269-271, Table 9-13
  0x5,		// bDescriptorType
  0x81,		// bEndpointAddress  1 | 0x80
  0x3,		// bmAttributes (0x03=intr)
  0xa,0x0,      // wMaxPacketSize
  0x5,          // bInterval max number of ms between transmit packets

}; 

const uint8_t hid_report_descriptor[154] = {
  0x5,0x1,                // USAGE_PAGE (Generic Desktop)
  0x9,0x2,                // USAGE (Mouse)
  0xa1,0x1,               // COLLECTION (Application)
  0x85,0x1,               //   REPORT_ID (1)
  0x9,0x1,                //   USAGE (Pointer)
  0xa1,0x0,               //   COLLECTION (Physical)
  0x5,0x9,                //     USAGE_PAGE (Button)
  0x19,0x1,               //     USAGE_MINIMUM (Button 1)
  0x29,0x3,               //     USAGE_MAXIMUM (Button 3)
  0x15,0x0,               //     LOGICAL_MINIMUM (0)
  0x25,0x1,               //     LOGICAL_MAXIMUM (1)
  0x95,0x3,               //     REPORT_COUNT (3)
  0x75,0x1,               //     REPORT_SIZE (1)
  0x81,0x2,               //     INPUT (Data,Var,Abs)
  0x95,0x5,               //     REPORT_COUNT (5)
  0x81,0x3,               //     INPUT (Cnst,Var,Abs)
  0x5,0x1,                //     USAGE_PAGE (Generic Desktop)
  0x9,0x30,               //     USAGE (X)
  0x9,0x31,               //     USAGE (Y)
  0x9,0x38,               //     USAGE (Wheel)
  0x15,0x81,              //     LOGICAL_MINIMUM (-127)
  0x25,0x7f,              //     LOGICAL_MAXIMUM (127)
  0x75,0x8,               //     REPORT_SIZE (8)
  0x95,0x3,               //     REPORT_COUNT (3)
  0x81,0x6,               //     INPUT (Data,Var,Rel)
  0xc0,           //   END_COLLECTION
  0xc0,           // END_COLLECTION
  0x5,0xd,                // USAGE_PAGE (Digitizers)
  0x9,0x1,                // USAGE (Digitizer)
  0xa1,0x1,               // COLLECTION (Application)
  0x85,0x2,               //   REPORT_ID (2)
  0x9,0x0,                //   USAGE (Undefined)
  0x75,0x8,               //   REPORT_SIZE (8)
  0x95,0x9,               //   REPORT_COUNT (9)
  0x15,0x0,               //   LOGICAL_MINIMUM (0)
  0x26,0xff,0x0,          //   LOGICAL_MAXIMUM (255)
  0x81,0x2,               //   INPUT (Data,Var,Abs)
  0x9,0x3a,               //   USAGE (Program Change Keys)
  0x25,0x2,               //   LOGICAL_MAXIMUM (2)
  0x95,0x1,               //   REPORT_COUNT (1)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x3,               //   REPORT_ID (3)
  0x9,0x0,                //   USAGE (Undefined)
  0x26,0xff,0x0,          //   LOGICAL_MAXIMUM (255)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x4,               //   REPORT_ID (4)
  0x9,0x3a,               //   USAGE (Program Change Keys)
  0x25,0x1,               //   LOGICAL_MAXIMUM (1)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x5,               //   REPORT_ID (5)
  0x9,0x0,                //   USAGE (Undefined)
  0x26,0xff,0x0,          //   LOGICAL_MAXIMUM (255)
  0x95,0x8,               //   REPORT_COUNT (8)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x6,               //   REPORT_ID (6)
  0x9,0x0,                //   USAGE (Undefined)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x7,               //   REPORT_ID (7)
  0x9,0x0,                //   USAGE (Undefined)
  0x95,0x4,               //   REPORT_COUNT (4)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x8,               //   REPORT_ID (8)
  0x9,0x0,                //   USAGE (Undefined)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0x9,               //   REPORT_ID (9)
  0x9,0x0,                //   USAGE (Undefined)
  0x95,0x1,               //   REPORT_COUNT (1)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0xa,               //   REPORT_ID (10)
  0x9,0x0,                //   USAGE (Undefined)
  0x95,0x2,               //   REPORT_COUNT (2)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0x85,0xb,               //   REPORT_ID (11)
  0x9,0x0,                //   USAGE (Undefined)
  0x95,0x1,               //   REPORT_COUNT (1)
  0xb1,0x2,               //   FEATURE (Data,Var,Abs)
  0xc0            // END_COLLECTION
};

const usb_string_t string0 =      { 0x04, 0x03, { 0x0409 } };
const usb_string_t string1 =      { 0x0c, 0x03, L"WACOM" }; //0x5700, 0x4100, 0x4300, 0x4f00, 0x4d00 };
usb_string_t string2 =      { 0x14, 0x03, L"XD-XXXX-U" };
const usb_string_t empty_string = { 0x04, 0x03, L"?" };

// Do product identification
//
void identify_product(uint16_t product_id) {
  for (uint8_t i = 0; i < n_product_ids; i++) {
    product_id_t * pid = &product_ids[i];
    if (pid->product_id == product_id) {
      // Set up target device descriptor
      device_descriptor[10] = (uint8_t)(pid->product_id >> 8);
      device_descriptor[11] = (uint8_t)(pid->product_id & 0xff);
      memcpy((void *)(string2.string), (const void *)(pid->product_name), string2.length - 2);
      return;
    }
  }
}

// Private stuff
wacom_report_t usb_report;

// queue an update / proximity message
void queue_message(message_type_t type, uint8_t index) {

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    switch (type) {
    case TOOL_IN:
      populate_in_proximity(index, &usb_report);
      break;
    case TOOL_OUT:
      populate_out_of_proximity(index, &usb_report);
      break;
    case TOOL_UPDATE:
      populate_update(index, &usb_report);
      break;
    default:
      break;
    }
  }
  LED_TOGGLE;
  usb_send_packet(&usb_report, 10, WACOM_INTUOS2_PEN_ENDPOINT, 25);
  LED_TOGGLE;
}

void populate_in_proximity(uint8_t index, wacom_report_t * packet) {
  // zero everything
  packet->bytes[0] = 0x02;

  // enter proximity packet type
  packet->bytes[1] = 0xc2 | (index & 0x01);
  
  // Transducers id
  // This is a 24 bit value, bits 12-15 are zero
  // bits 12-23 ony exist for intuos / cintiq models after intuos 4
  packet->bytes[7] = transducers[index].type >> 20;           // bits 20-23
  packet->bytes[8] = (transducers[index].type >> 12) & 0xf0;  // bits 16-19
  packet->bytes[2] = transducers[index].type >> 4;            // bits 4-11
  packet->bytes[3] = transducers[index].type << 4;            // bits 0-3

  // Transducers serial number
  // 32 bit value
  packet->bytes[3] |= (transducers[index].id >> 28) & 0x0f;  // bits 28-31
  packet->bytes[4] = transducers[index].id >> 20;            // bits 20-27
  packet->bytes[5] = transducers[index].id >> 12;            // bits 12-19
  packet->bytes[6] = transducers[index].id >> 4;             // bits 4-11
  packet->bytes[7] |= transducers[index].id << 4;            // bits 0-3

  // Zero last byte
  packet->bytes[9] = 0x00;
}

void populate_out_of_proximity(uint8_t index, wacom_report_t * packet) {
  // zero everything
  memset(packet, 0, sizeof(wacom_report_t));
  packet->bytes[0] = 0x02;

  // exit proximity packet type
  packet->bytes[1] = 0x80 | (index & 0x01);
}

void populate_update(uint8_t index, wacom_report_t * packet) {
  uint16_t transformed;
  // zero everything
  memset(packet, 0, sizeof(wacom_report_t));
  packet->bytes[0] = 0x02;

  // Stuff that doesn't require a transform
  packet->tool_index = index;
  packet->proximity = 1; //transducers[index].touching;

  if (transducers[index].touching) {
    packet->distance = 0x0d;
  } else {
    packet->distance = 0x1f;
  }

  // Transformed data
  transformed = location_to_location(transducers[index].location_x);
  packet->x_hi = transformed >> 8;
  packet->x_lo = transformed & 0xff;

  transformed = location_to_location(transducers[index].location_y);
  packet->y_hi = transformed >> 8;
  packet->y_lo = transformed & 0xff;

  // Now we've done the generic update stuff, time to go tool-specific
  switch (transducers[index].type & 0xff7) {
  case STYLUS_STANDARD:
    // populate pen buttons
    packet->bytes[1] |= (transducers[index].buttons & 0x03) << 1;
    // And the mysterious top 2 bits
    packet->bytes[1] |= 0xc0;

    // pressure reading
    transformed = pressure_to_pressure(transducers[index].pressure);
    packet->payload[0] = transformed >> 2;
    packet->payload[1] = transformed << 6;

    transformed = tilt_to_tilt(transducers[index].tilt_x);
    packet->payload[1] |= transformed >> 1;
    packet->payload[2] = transformed << 7;

    transformed = tilt_to_tilt(transducers[index].tilt_y);
    packet->payload[2] |= transformed;

    break;
  case MOUSE_4D:
    // Tell driver what packet we are.
    packet->bit7 = 1;
    packet->bit6 = 1;
    packet->proximity = 1;
    packet->bit3 = 1;

    if (transducers[index].output_state == 0) {
      // 4D Mouse second packet - rotation
      transformed = rotation_to_rotation(transducers[index].rotation, transducers[index].rotation_sign);

      packet->payload[0] = transformed >> 3;
      packet->payload[1] = transformed << 5;
     
      // Swap state
      transducers[index].output_state = 1;
    } else {
      // Deal with z sign here
      // Tablet reports -1024 <= z <= 1023
      // Driver wants 10 bit magnitude in payload 0-1, and a separate sign bit interleaved with the biuttons  
      
      transformed = z_to_z(transducers[index].z);

      packet->payload[0] = transformed >> 3;
      packet->payload[1] = (transformed & 0x7fe) << 5;

      // buttons are not laid out the same way as the serial tablets report them.
      // In fact, the buttons are already laid out ready for the USB report, i.e
      // [xxxx xxxx xx45 x321] meaning we can simply interpose the z sign bit
      packet->payload[2] = (transducers[index].buttons & 0x37) | ((transformed & 0x01) << 3);

      // Swap state
      transducers[index].output_state = 0;
    }
    packet->bit1 = transducers[index].output_state;

    break;
  default:
    while(1) {
      signal_pause();
      signal_word_bcd(transducers[index].type & 0xffff, 0);
    }
    break;
  }
}    
