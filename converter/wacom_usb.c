#include <stdint.h>
#include <avr/pgmspace.h>
#include "usb_side.h"

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
const uint8_t PROGMEM config_descriptor[34] = {
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
  0x5		// bInterval max number of ms between transmit packets
}; 

const uint8_t PROGMEM hid_report_descriptor[154] = {
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
    0x04, 0x03, 
    '?', 0x00
  };
