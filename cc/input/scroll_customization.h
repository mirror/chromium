// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLL_CUSTOMIZATION_H_
#define CC_INPUT_SCROLL_CUSTOMIZATION_H_

#include <cstdlib>
#include <string>

#include "base/logging.h"

namespace cc {

enum ScrollCustomization {
  // No scrolling or zooming allowed.
  kScrollCustomizationNone = 0x0,
  kScrollCustomizationPanLeft = 0x1,
  kScrollCustomizationPanRight = 0x2,
  kScrollCustomizationPanX =
      kScrollCustomizationPanLeft | kScrollCustomizationPanRight,
  kScrollCustomizationPanUp = 0x4,
  kScrollCustomizationPanDown = 0x8,
  kScrollCustomizationPanY =
      kScrollCustomizationPanUp | kScrollCustomizationPanDown,
  kScrollCustomizationAuto =
      kScrollCustomizationPanX | kScrollCustomizationPanY,
  kScrollCustomizationMax = (1 << 4) - 1
};

inline ScrollCustomization operator|(ScrollCustomization a,
                                     ScrollCustomization b) {
  return static_cast<ScrollCustomization>(int(a) | int(b));
}

inline ScrollCustomization& operator|=(ScrollCustomization& a,
                                       ScrollCustomization b) {
  return a = a | b;
}

inline ScrollCustomization operator&(ScrollCustomization a,
                                     ScrollCustomization b) {
  return static_cast<ScrollCustomization>(int(a) & int(b));
}

inline ScrollCustomization& operator&=(ScrollCustomization& a,
                                       ScrollCustomization b) {
  return a = a & b;
}

inline const char* ScrollCustomizationToString(
    ScrollCustomization touch_action) {
  switch (static_cast<int>(touch_action)) {
    case 0:
      return "NONE";
    case 1:
      return "PAN_LEFT";
    case 2:
      return "PAN_RIGHT";
    case 3:
      return "PAN_X";
    case 4:
      return "PAN_UP";
    case 5:
      return "PAN_LEFT_PAN_UP";
    case 6:
      return "PAN_RIGHT_PAN_UP";
    case 7:
      return "PAN_X_PAN_UP";
    case 8:
      return "PAN_DOWN";
    case 9:
      return "PAN_LEFT_PAN_DOWN";
    case 10:
      return "PAN_RIGHT_PAN_DOWN";
    case 11:
      return "PAN_X_PAN_DOWN";
    case 12:
      return "PAN_Y";
    case 13:
      return "PAN_LEFT_PAN_Y";
    case 14:
      return "PAN_RIGHT_PAN_Y";
    case 15:
      return "AUTO";
  }
  NOTREACHED();
  return "";
}

}  // namespace cc

#endif  // CC_INPUT_SCROLL_CUSTOMIZATION_H_
