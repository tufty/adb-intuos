#include <stdint.h>
#include <avr/pgmspace.h>
#include "usb_side.h"

// Tablet device descriptor
// Pre-set up with the stuff we already know
uint8_t device_descriptor[18] = 
  { 18, 1, 0x00, 0x02, 0, 0, 0, 8, 0x6a, 0x05, 0x42, 0x00, 0x26, 0x01, 1, 2, 0, 1};

// Config and HID report descriptors
// Ripped direct from Bernard's Intuos 2 descriptors.
// These are static
const uint8_t PROGMEM config_descriptor[34] = 
  { 
    0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,
    0x09, 0x04, 0x00, 0x00, 0x01, 0x03, 0x01, 0x01, 0x00,
    0x09, 0x21, 0x00, 0x01, 0x00, 0x01, 0x22, 0x9a, 0x00,
    0x07, 0x05, 0x81, 0x03, 0x0a, 0x00, 0x05
  };

const uint8_t PROGMEM hid_report_descriptor[154] =
  {
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

const uint8_t PROGMEM string0[4] = { 4, 3, 0x04,0x09 };

const uint8_t PROGMEM string1[12] = 
  {
    0x0c, 0x03,
    'W', 0x00, 'A', 0x00, 'C', 0x00, 'O', 0x00, 'M', 0x00
  };
  
uint8_t string2[20] = 
  {
    0x14, 0x03,
    'X', 0x00, 'D', 0x00, '-', 0x00, 'X', 0x00, 'X', 0x00, 'X', 0x00, 'X', 0x00, '-', 0x00, 'U', 0x00
  };

const uint8_t empty_string[2] = 
  {
    0x02, 0x03
  };
