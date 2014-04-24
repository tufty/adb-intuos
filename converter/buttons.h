#ifndef __BUTTONS_H__
#define __BUTTONS_H__
// Copyright (c) 2013-2014 by Simon Stapleton (simon.stapleton@gmail.com)
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


// The goddamn button bar is a massive pain in the ass.
// For Intuos 1 -> Intuos 2, everything should just work.
// For Ultrapad -> Intuos 2, we need to rescale the tablet readings to fit the "active" 
// tablet area, then synthesize button clicks.
// For Calcomps, we need to rescale the tablet readings.
// For Intuos 1 -> Intuos 3+, we need to rescale the tablet readings to fit the active 
// area, and then detect button events, synthesizing "physical" button presses.
// For Ultrapad -> Intuos 3+, we only need to synthesize physical button presses.
// This means we need, one way or another:
// - A "rescale" function which either compresses a full tablet to an "inset", or vice versa.
// - A list of tablet button co-ordinates.
//
// For intuos, the buttons appear in (up to) 5 groups.
// 6 "file menu" buttons, (up to) 5 "edit menu" buttons, n "additional" buttons, 2 "mode" buttons
// and 3 "pressure" buttons".
// GD-0405 has : file, cut/copy/paste, pen/mouse.  They don't apear to be "in" the active area.
// GD-0608 has : file, edit, f12->f13, pen/mouse, soft/med/firm
// GD-0912 has : file, edit, f12->f16, pen/mouse, soft/med/firm
// GD-1212 has : file, edit, f12->f16, pen/mouse, soft/med/firm
// GD-1218 has : file, edit, f12->f27, pen/mouse, soft/med/firm
// 
// On the 0608 model, the button bar is ~9mm high.  On the 1212 model, ~11mm.
// Interbutton and interblock spacing appears to be the same between models, with
// button width being the same as bar height. 
// On the GD-0608, buttons are ~8.5mm by 5mm, vertically centred, with ~2mm
// spacing.
// Blocks start and end at ~3mm from tablet edge
// Interblock spacing is ~4mm
// My fucking caliper is crap.
//
// We don't know exactly the measures, but we have 4 variables and 4 equations...
// Unsurprisingly, this doesn't solve, as the sizes vary by model.  Bloody Wacom.
// 2a + 2b + 8c + 10x = 122.5
// 2a + 4b + 13c + 18x = 196
// 2a + 4b + 16c + 21x = 294
// 2a + 4b + 27c + 32x = 441
//
// Bernard's templates imply that UD-0608 has 880 points less resolution in the y 
// axis than the GD-0608, which implies 8.8 mm for the button bar.
// UD-0912 is the same if we believe Bernard. I'm not sure I do.
// UD-1212 / UD1218 has 1200 points less, 12mm.  Feasible.

// the hell with it.  Remeasure and average
// GD-0608
// Button width : 8.8mm
// interbutton : 1.7mm
// side : 3.2mm
// interblock : 4.1mm

typedef struct {
  enum {
    BTN_NONE, BTN_NEW, BTN_OPEN, BTN_CLOSE, BTN_SAVE, BTN_PRINT, BTN_EXIT,
    BTN_CUT, BTN_COPY, BTN_PASTE, BTN_UNDO, BTN_DEL,
    BTN_F12, BTN_F13, BTN_F14, BTN_F15, BTN_F16, BTN_F17, BTN_F18, BTN_F19,
    BTN_F20, BTN_F21, BTN_F22, BTN_F23, BTN_F24, BTN_F25, BTN_F26, BTN_F27,
    BTN_PEN, BTN_MOUSE,
    BTN_SOFT, BTN_MED, BTN_FIRM
  } button;
  uint16_t x;
} button_t;

#define NONE {BTN_NONE, 0}

#endif
