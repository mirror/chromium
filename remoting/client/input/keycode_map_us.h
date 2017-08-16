// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_KEYCODE_MAP_US_H_
#define REMOTING_CLIENT_KEYCODE_MAP_US_H_

namespace remoting {

// A mapping for characters on US keyboard to Chromoting keycodes.

const int kKeyboardKeyMaxUS = 126;

// Index for specific keys
const uint32_t kShiftIndex = 128;
const uint32_t kBackspaceIndex = 129;
const uint32_t kCtrlIndex = 130;
const uint32_t kAltIndex = 131;
const uint32_t kDelIndex = 132;

struct KeyCodeMeta {
  uint32_t code;
  bool needsShift;
};

const KeyCodeMeta kAsciiToKeyCodeUS[] = {
    {0, false},         // [0]      Numbering fields by index, falset by count
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0x070028, false},  // [10]     ENTER
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         // [20]
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         //
    {0, false},         // [30]
    {0, false},         //
    {0x07002c, false},  //          SPACE
    {0x07001e, true},   //          !
    {0x070034, true},   //          "
    {0x070020, true},   //          #
    {0x070021, true},   //          $
    {0x070022, true},   //          %
    {0x070024, true},   //          &
    {0x070034, false},  //          '
    {0x070026, true},   // [40]     (
    {0x070027, true},   //          )
    {0x070025, true},   //          *
    {0x07002e, true},   //          +
    {0x070036, false},  //          ,
    {0x07002d, false},  //          -
    {0x070037, false},  //          .
    {0x070038, false},  //          /
    {0x070027, false},  //          0
    {0x07001e, false},  //          1
    {0x07001f, false},  // [50]     2
    {0x070020, false},  //          3
    {0x070021, false},  //          4
    {0x070022, false},  //          5
    {0x070023, false},  //          6
    {0x070024, false},  //          7
    {0x070025, false},  //          8
    {0x070026, false},  //          9
    {0x070033, true},   //          :
    {0x070033, false},  //          ;
    {0x070036, true},   // [60]     <
    {0x07002e, false},  //          =
    {0x070037, true},   //          >
    {0x070038, true},   //          ?
    {0x07001f, true},   //          @
    {0x070004, true},   //          A
    {0x070005, true},   //          B
    {0x070006, true},   //          C
    {0x070007, true},   //          D
    {0x070008, true},   //          E
    {0x070009, true},   // [70]     F
    {0x07000a, true},   //          G
    {0x07000b, true},   //          H
    {0x07000c, true},   //          I
    {0x07000d, true},   //          J
    {0x07000e, true},   //          K
    {0x07000f, true},   //          L
    {0x070010, true},   //          M
    {0x070011, true},   //          N
    {0x070012, true},   //          O
    {0x070013, true},   // [80]     P
    {0x070014, true},   //          Q
    {0x070015, true},   //          R
    {0x070016, true},   //          S
    {0x070017, true},   //          T
    {0x070018, true},   //          U
    {0x070019, true},   //          V
    {0x07001a, true},   //          W
    {0x07001b, true},   //          X
    {0x07001c, true},   //          Y
    {0x07001d, true},   // [90]     Z
    {0x07002f, false},  //          [
    {0x070031, false},  //          BACKSLASH
    {0x070030, false},  //          ]
    {0x070023, true},   //          ^
    {0x07002d, true},   //          _
    {0, false},         //
    {0x070004, false},  //          a
    {0x070005, false},  //          b
    {0x070006, false},  //          c
    {0x070007, false},  // [100]    d
    {0x070008, false},  //          e
    {0x070009, false},  //          f
    {0x07000a, false},  //          g
    {0x07000b, false},  //          h
    {0x07000c, false},  //          i
    {0x07000d, false},  //          j
    {0x07000e, false},  //          k
    {0x07000f, false},  //          l
    {0x070010, false},  //          m
    {0x070011, false},  // [110]    n
    {0x070012, false},  //          o
    {0x070013, false},  //          p
    {0x070014, false},  //          q
    {0x070015, false},  //          r
    {0x070016, false},  //          s
    {0x070017, false},  //          t
    {0x070018, false},  //          u
    {0x070019, false},  //          v
    {0x07001a, false},  //          w
    {0x07001b, false},  // [120]    x
    {0x07001c, false},  //          y
    {0x07001d, false},  //          z
    {0x07002f, true},   //          {
    {0x070031, true},   //          |
    {0x070030, true},   //          }
    {0x070035, true},   //          ~
    {0, false},         // [127]
    {0x0700e1, false},  //          SHIFT
    {0x07002a, false},  //          BACKSPACE
    {0x0700e0, false},  //          CTRL
    {0x0700e2, false},  //          ALT
    {0x07004c, false}   //          DEL
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_KEYCODE_MAP_US_H_
