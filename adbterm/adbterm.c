/* Nasty hacky serial->adb converter
 * Copyright (c) 2013 Simon Stapleton, portions (c) 2008 PJRC.COM, LTD
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <ctype.h>
#include <util/delay.h>
#include "avr_util.h"
#include "usb_serial.h"
#include "adb.h"

#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD |= (1<<6))
#define LED_OFF		(PORTD &= ~(1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

// Nasty globals
bool binary_output = true;
bool hex_output = true;
bool adb_command_just_finished = false;
bool adb_command_queued = false;
void (*callback)(uint8_t) = 0;
AdbPacket the_packet;
uint8_t adb_command = ADB_COMMAND_TALK;
uint8_t adb_register = 0;
uint8_t adb_address = 4;
uint8_t n_adb_bytes = 0;
uint8_t adb_bytes[8] = {0,0,0,0,0,0,0,0};

void send_str(const char *s);
uint8_t recv_str(char *buf, uint8_t size);
void parse_and_execute_command(const char *buf, uint8_t num);
void dump_adb_packet(void);
char itoh (uint8_t i);

// Basic command interpreter for controlling port pins
int main(void)
{
  char buf[32];
  uint8_t n;

  // set for 16 MHz clock, and turn on the LED
  CPU_PRESCALE(0);
  LED_CONFIG;
  LED_ON;

  // initialize the USB, and then wait for the host
  // to set configuration.  If the Teensy is powered
  // without a PC connected to the USB port, this 
  // will wait forever.
  usb_init();
  while (!usb_configured()) /* wait */ ;
  _delay_ms(1000);
  adb_init();

  while (1) {
    // wait for the user to run their terminal emulator program
    // which sets DTR to indicate it is ready to receive.
    while (!(usb_serial_get_control() & USB_SERIAL_DTR)) /* wait */ ;

    // discard anything that was received prior.  Sometimes the
    // operating system or other software will send a modem
    // "AT command", which can still be buffered.
    usb_serial_flush_input();

    // print a nice welcome message
    send_str(PSTR("\r\nADB terminal\r\n\r\n"));
    send_str(PSTR("\r\nCommands:\r\nR        : ADB Bus Reset\r\nF        : ADB Flush\r\n[d]T[n]  : Device d Talk Register n\r\n[d]Ln:x     : Device d Listen Register n data x\r\nP        : Poll last command until ^C\r\n<Return> : Repeat last command\r\n%        : Toggle binary output\r\n#        : Toggle hex output\r\n\r\n"));

    // and then listen for commands and process them
    while (1) {
      if (adb_command_just_finished) {
	adb_command_just_finished = false;
	if (the_packet.datalen > 0 || !adb_command_queued)
	  dump_adb_packet();
      }
      
       if (adb_command_queued && usb_serial_available() == 0) {
	uint8_t i = 0;

	adb_command_queued = false;

	the_packet.datalen = n_adb_bytes;

	for (i = 0; i < n_adb_bytes; i++) {
	  the_packet.data[i] = adb_bytes[i];
	}
	for (/**/; i < 8; i++) {
	  the_packet.data[i] = 0;
	}
	the_packet.command = adb_command;
	the_packet.address = adb_address;
	the_packet.parameter = adb_register;
  
	initiateAdbTransfer(&the_packet, callback);
	_delay_ms(8);
      } else {
	send_str(PSTR("> "));
	n = recv_str(buf, sizeof(buf));
	if (n == 255) break;
	send_str(PSTR("\r\n"));
	parse_and_execute_command(buf, n);
      }
    }
  }
}

// Send a string to the USB serial port.  The string must be in
// flash memory, using PSTR
//
void send_str(const char *s)
{
  char c;
  while (1) {
    c = pgm_read_byte(s++);
    if (!c) break;
    usb_serial_putchar(c);
  }
}

// Receive a string from the USB serial port.  The string is stored
// in the buffer and this function will not exceed the buffer size.
// A carriage return or newline completes the string, and is not
// stored into the buffer.
// The return value is the number of characters received, or 255 if
// the virtual serial connection was closed while waiting.
//
uint8_t recv_str(char *buf, uint8_t size)
{
  int16_t r;
  uint8_t count=0;

  while (count < size) {
    r = usb_serial_getchar();
    if (r != -1) {
      if (r == '\r' || r == '\n') return count;
      if (r >= ' ' && r <= '~') {
	*buf++ = r;
	usb_serial_putchar(r);
	count++;
      }
    } else {
      if (!usb_configured() ||
	  !(usb_serial_get_control() & USB_SERIAL_DTR)) {
	// user no longer connected
	return 255;
      }
      // just a normal timeout, keep waiting
    }
  }
  return count;
}

uint8_t htoi (char the_char) {
  if (the_char <= '9') 
    return the_char - '0';
  if (the_char <= 'F')
    return the_char - 'A' + 10;
  else
    return the_char - 'a' + 10;
}

char itoh (uint8_t i) {
  if (i >= 10)
    return 'a' + i - 10;
  else
    return '0' + i;
}

void dump_adb_packet() {
  if (adb_command == ADB_COMMAND_RESET) {
    send_str(PSTR("Reset\r\n"));
    return;
  }
    
  usb_serial_putchar('[');
  usb_serial_putchar(itoh(adb_address));
  switch (adb_command) {
  case ADB_COMMAND_TALK:
    usb_serial_putchar('T'); break;
  case ADB_COMMAND_LISTEN:
    usb_serial_putchar('L'); break;
  }
  usb_serial_putchar(itoh(adb_register));
  usb_serial_putchar(']');

  if (the_packet.datalen > 0) {
    if (binary_output) {
      for (int i = 0; i < the_packet.datalen; i++) {
	for (int j = 7; j >= 0; j--) {
	  if (((the_packet.data[i] >> j) & 1) == 1)
	    usb_serial_putchar('1');
	  else
	    usb_serial_putchar('0');
	}
	usb_serial_putchar(' ');
      }
    }
    if (hex_output) {
      for (int i = 0; i < the_packet.datalen; i++) {
	usb_serial_putchar(itoh(the_packet.data[i] >> 4));
	usb_serial_putchar(itoh(the_packet.data[i] & 0x0f));
      }
      usb_serial_putchar(' ');
    }
  }
  send_str(PSTR("\r\n"));
}


void single_command_callback(uint8_t errorCode) {
  adb_command_just_finished = true;
}

void poll_callback(uint8_t errorCode) {
  adb_command_just_finished = true;
  adb_command_queued = true;
}


// parse a user command and execute it, or print an error message
//
void parse_and_execute_command(const char *buf, uint8_t num)
{
  callback = &single_command_callback;

  n_adb_bytes = 0;
  for (int i = 0; i < 8; i++) adb_bytes[i] = 0;

  char* the_char = (char *)buf;

  if (isxdigit(*the_char)) {
    adb_address = htoi(*the_char);
    the_char++;
  }
 
  while ((the_char < buf + num) && (*the_char != ':')) {
    switch (*the_char) {
    case '%':
      binary_output = !binary_output;
      break;
    case '#':
      hex_output = !hex_output;
      break;
    case 'T':
      adb_command = ADB_COMMAND_TALK;
      break;
    case 'L':
      adb_command = ADB_COMMAND_LISTEN;
      break;
    case 'R':
      adb_command = ADB_COMMAND_RESET;
      break;
    case 'F':
      adb_command = ADB_COMMAND_FLUSH;
      break;
    case 'P':
      callback = &poll_callback;
      break;
    default:
      if (isdigit(*the_char)) {
	adb_register = htoi(*the_char);
      }
    }
    the_char ++;
  }
    
  if ((*the_char = ':') && (the_char != buf + num)) {
    the_char ++;
    while (the_char < buf + num) {
      n_adb_bytes = (buf + num - the_char) >> 1;
      adb_bytes[7] = (adb_bytes[7] << 4) | (adb_bytes[6] >> 4);
      adb_bytes[7] = (adb_bytes[6] << 4) | (adb_bytes[5] >> 4);
      adb_bytes[7] = (adb_bytes[5] << 4) | (adb_bytes[4] >> 4);
      adb_bytes[7] = (adb_bytes[4] << 4) | (adb_bytes[3] >> 4);
      adb_bytes[7] = (adb_bytes[3] << 4) | (adb_bytes[2] >> 4);
      adb_bytes[7] = (adb_bytes[2] << 4) | (adb_bytes[1] >> 4);
      adb_bytes[7] = (adb_bytes[1] << 4) | (adb_bytes[0] >> 4);
      adb_bytes[7] = (adb_bytes[0] << 4) | htoi(*the_char);
      the_char ++;
    }
  }

  the_packet.command = adb_command;
  the_packet.address = adb_address;
  the_packet.parameter = adb_register;

  adb_command_queued = true;
}


