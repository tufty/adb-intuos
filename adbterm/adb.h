/*
 * adb_codec.h
 *
 *  Created on: 2010-11-13
 *      Author: Bernard
 */

#ifndef ADB_CODEC_H_
#define ADB_CODEC_H_


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
    byte headerRawByte;
  };
  byte data[8];
  byte datalen;
} AdbPacket;

void adb_init(void);
void initiateAdbTransfer(AdbPacket* adbPacket, void (*done_callback)(uint8_t errorCode));

#define ADB_COMMAND_RESET 	0
#define ADB_COMMAND_FLUSH       1
#define ADB_COMMAND_LISTEN 	2
#define ADB_COMMAND_TALK 	3


#endif /* ADB_CODEC_H_ */
