#ifndef __ADB_CODEC_H__
#define __ADB_CODEC_H__

// Copyright (c) 2010, Bernard Poulin (bernard-at-acm-dot-org) as part of the 
// Waxbee project
// Conversion from C++ (c) 2013, Simon Stapleton (simon.stapleton@gmail.com)
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

typedef struct {
  union
  {
    struct
    {
      // note: bit order is msb first
      unsigned int parameter:2;
      unsigned int command:2;
      unsigned int address:4;
    };
    uint8_t headerRawByte;
  };
  uint8_t data[8];
  uint8_t datalen;
} AdbPacket;

void adb_init(void);
void initiateAdbTransfer(volatile AdbPacket* adbPacket, void (*done_callback)(uint8_t errorCode));

#define ADB_COMMAND_RESET 	0
#define ADB_COMMAND_FLUSH       1
#define ADB_COMMAND_LISTEN 	2
#define ADB_COMMAND_TALK 	3

#define ADB_REGISTER_0          0
#define ADB_REGISTER_1          1
#define ADB_REGISTER_2          2
#define ADB_REGISTER_3          3

#endif /* __ADB_CODEC_H__ */
