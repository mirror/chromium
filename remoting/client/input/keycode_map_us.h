// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_INPUT_KEYCODE_MAP_US_H_
#define REMOTING_CLIENT_INPUT_KEYCODE_MAP_US_H_

#include "ui/events/keycodes/dom/dom_code.h"

namespace remoting {

const int kKeyboardKeyMaxUS = 126;

struct KeyCodeMeta {
  ui::DomCode code;
  bool needsShift;
};

// This array maps an ASCII character to the corresponding DOM code on the US
// layout keyboard.
//
// Usage:
//   keyCodeMeta = kAsciiToKeyCodeUS[ascii]
//   ascii >= 0 && ascii <= kKeyboardKeyMaxUS
const KeyCodeMeta kAsciiToKeyCodeUS[] = {
    {ui::DomCode::NONE, false},           // [0]
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::ENTER, false},          // [10]     ENTER
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           // [20]
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::ESCAPE, false},         //          ESCAPE
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::NONE, false},           // [30]
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::SPACE, false},          //          SPACE
    {ui::DomCode::DIGIT1, true},          //          !
    {ui::DomCode::QUOTE, true},           //          "
    {ui::DomCode::DIGIT3, true},          //          #
    {ui::DomCode::DIGIT4, true},          //          $
    {ui::DomCode::DIGIT5, true},          //          %
    {ui::DomCode::DIGIT7, true},          //          &
    {ui::DomCode::QUOTE, false},          //          '
    {ui::DomCode::DIGIT9, true},          // [40]     (
    {ui::DomCode::DIGIT0, true},          //          )
    {ui::DomCode::DIGIT8, true},          //          *
    {ui::DomCode::EQUAL, true},           //          +
    {ui::DomCode::COMMA, false},          //          ,
    {ui::DomCode::MINUS, false},          //          -
    {ui::DomCode::PERIOD, false},         //          .
    {ui::DomCode::SLASH, false},          //          /
    {ui::DomCode::DIGIT0, false},         //          0
    {ui::DomCode::DIGIT1, false},         //          1
    {ui::DomCode::DIGIT2, false},         // [50]     2
    {ui::DomCode::DIGIT3, false},         //          3
    {ui::DomCode::DIGIT4, false},         //          4
    {ui::DomCode::DIGIT5, false},         //          5
    {ui::DomCode::DIGIT6, false},         //          6
    {ui::DomCode::DIGIT7, false},         //          7
    {ui::DomCode::DIGIT8, false},         //          8
    {ui::DomCode::DIGIT9, false},         //          9
    {ui::DomCode::SEMICOLON, true},       //          :
    {ui::DomCode::SEMICOLON, false},      //          ;
    {ui::DomCode::COMMA, true},           // [60]     <
    {ui::DomCode::EQUAL, false},          //          =
    {ui::DomCode::PERIOD, true},          //          >
    {ui::DomCode::SLASH, true},           //          ?
    {ui::DomCode::DIGIT2, true},          //          @
    {ui::DomCode::US_A, true},            //          A
    {ui::DomCode::US_B, true},            //          B
    {ui::DomCode::US_C, true},            //          C
    {ui::DomCode::US_D, true},            //          D
    {ui::DomCode::US_E, true},            //          E
    {ui::DomCode::US_F, true},            // [70]     F
    {ui::DomCode::US_G, true},            //          G
    {ui::DomCode::US_H, true},            //          H
    {ui::DomCode::US_I, true},            //          I
    {ui::DomCode::US_J, true},            //          J
    {ui::DomCode::US_K, true},            //          K
    {ui::DomCode::US_L, true},            //          L
    {ui::DomCode::US_M, true},            //          M
    {ui::DomCode::US_N, true},            //          N
    {ui::DomCode::US_O, true},            //          O
    {ui::DomCode::US_P, true},            // [80]     P
    {ui::DomCode::US_Q, true},            //          Q
    {ui::DomCode::US_R, true},            //          R
    {ui::DomCode::US_S, true},            //          S
    {ui::DomCode::US_T, true},            //          T
    {ui::DomCode::US_U, true},            //          U
    {ui::DomCode::US_V, true},            //          V
    {ui::DomCode::US_W, true},            //          W
    {ui::DomCode::US_X, true},            //          X
    {ui::DomCode::US_Y, true},            //          Y
    {ui::DomCode::US_Z, true},            // [90]     Z
    {ui::DomCode::BRACKET_LEFT, false},   //          [
    {ui::DomCode::BACKSLASH, false},      //          BACKSLASH
    {ui::DomCode::BRACKET_RIGHT, false},  //          ]
    {ui::DomCode::DIGIT6, true},          //          ^
    {ui::DomCode::MINUS, true},           //          _
    {ui::DomCode::NONE, false},           //
    {ui::DomCode::US_A, false},           //          a
    {ui::DomCode::US_B, false},           //          b
    {ui::DomCode::US_C, false},           //          c
    {ui::DomCode::US_D, false},           // [100]    d
    {ui::DomCode::US_E, false},           //          e
    {ui::DomCode::US_F, false},           //          f
    {ui::DomCode::US_G, false},           //          g
    {ui::DomCode::US_H, false},           //          h
    {ui::DomCode::US_I, false},           //          i
    {ui::DomCode::US_J, false},           //          j
    {ui::DomCode::US_K, false},           //          k
    {ui::DomCode::US_L, false},           //          l
    {ui::DomCode::US_M, false},           //          m
    {ui::DomCode::US_N, false},           // [110]    n
    {ui::DomCode::US_O, false},           //          o
    {ui::DomCode::US_P, false},           //          p
    {ui::DomCode::US_Q, false},           //          q
    {ui::DomCode::US_R, false},           //          r
    {ui::DomCode::US_S, false},           //          s
    {ui::DomCode::US_T, false},           //          t
    {ui::DomCode::US_U, false},           //          u
    {ui::DomCode::US_V, false},           //          v
    {ui::DomCode::US_W, false},           //          w
    {ui::DomCode::US_X, false},           // [120]    x
    {ui::DomCode::US_Y, false},           //          y
    {ui::DomCode::US_Z, false},           //          z
    {ui::DomCode::BRACKET_LEFT, true},    //          {
    {ui::DomCode::BACKSLASH, true},       //          |
    {ui::DomCode::BRACKET_RIGHT, true},   //          }
    {ui::DomCode::BACKQUOTE, true},       //          ~
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_INPUT_KEYCODE_MAP_US_H_
