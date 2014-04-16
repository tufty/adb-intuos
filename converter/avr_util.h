#ifndef __AVR_UTIL_H__
#define __AVR_UTIL_H__

// Copyright (c) 2010 Bernard Poulin (bernard-at-acm-dot-org) as part of the 
// Waxbee project
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

#include <avr/io.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#ifndef tbi
// toggle bit
#define tbi(sfr, bit) (_SFR_BYTE(sfr) ^= _BV(bit))
#endif

#ifndef HIGH
#define HIGH	1
#endif
#ifndef LOW
#define LOW	0
#endif

// define handy "byte" type
typedef uint8_t byte;

#ifndef __cplusplus

#ifndef true
#define true	1
#endif

#ifndef false
#define false	0
#endif

#ifndef bool
#define bool uint8_t
#endif

#endif

#define BITV(bit, val)  (val << bit)

/** Suppresses all interrupts */
#define __BEGIN_CRITICAL_SECTION \
	{\
		uint8_t save_statusreg = SREG;\
		cli();

/** Restore global interrupt status */
#define __END_CRITICAL_SECTION \
		SREG = save_statusreg;\
	}


#endif /* AVR_UTIL_H_ */
