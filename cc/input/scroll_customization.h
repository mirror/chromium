// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLL_CUSTOMIZATION_H_
#define CC_INPUT_SCROLL_CUSTOMIZATION_H_

#include "base/logging.h"
#include "cc/cc_export.h"

namespace cc {

enum ScrollCustomizationEnabledDirection : uint8_t {
  // No scrolling or zooming allowed.
  kNone = 0x0,
  kPanLeft = 0x1,
  kPanRight = 0x2,
  kPanX = kPanLeft | kPanRight,
  kPanUp = 0x4,
  kPanDown = 0x8,
  kPanY = kPanUp | kPanDown,
  kAuto = kPanX | kPanY,
  kMax = (1 << 4) - 1
};

inline ScrollCustomizationEnabledDirection operator|(
    ScrollCustomizationEnabledDirection a,
    ScrollCustomizationEnabledDirection b) {
  return static_cast<ScrollCustomizationEnabledDirection>(int(a) | int(b));
}

inline ScrollCustomizationEnabledDirection& operator|=(
    ScrollCustomizationEnabledDirection& a,
    ScrollCustomizationEnabledDirection b) {
  return a = a | b;
}

inline ScrollCustomizationEnabledDirection operator&(
    ScrollCustomizationEnabledDirection a,
    ScrollCustomizationEnabledDirection b) {
  return static_cast<ScrollCustomizationEnabledDirection>(int(a) & int(b));
}

inline ScrollCustomizationEnabledDirection& operator&=(
    ScrollCustomizationEnabledDirection& a,
    ScrollCustomizationEnabledDirection b) {
  return a = a & b;
}

inline const char* ScrollCustomizationToString(
    ScrollCustomizationEnabledDirection scroll_customization) {
  switch (static_cast<int>(scroll_customization)) {
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

// Returns the scroll customization which is compatible with the provided
// direction deltas.
CC_EXPORT ScrollCustomizationEnabledDirection
GetScrollCustomizationForDirection(double delta_x, double delta_y);

}  // namespace cc

#endif  // CC_INPUT_SCROLL_CUSTOMIZATION_H_
