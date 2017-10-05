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

inline constexpr ScrollCustomizationEnabledDirection operator|(
    ScrollCustomizationEnabledDirection a,
    ScrollCustomizationEnabledDirection b) {
  return static_cast<ScrollCustomizationEnabledDirection>(int(a) | int(b));
}

inline constexpr ScrollCustomizationEnabledDirection& operator|=(
    ScrollCustomizationEnabledDirection& a,
    ScrollCustomizationEnabledDirection b) {
  return a = a | b;
}

inline constexpr ScrollCustomizationEnabledDirection operator&(
    ScrollCustomizationEnabledDirection a,
    ScrollCustomizationEnabledDirection b) {
  return static_cast<ScrollCustomizationEnabledDirection>(int(a) & int(b));
}

inline constexpr ScrollCustomizationEnabledDirection& operator&=(
    ScrollCustomizationEnabledDirection& a,
    ScrollCustomizationEnabledDirection b) {
  return a = a & b;
}

inline const char* ScrollCustomizationToString(
    ScrollCustomizationEnabledDirection scroll_customization) {
  switch (static_cast<int>(scroll_customization)) {
    case kNone:
      return "NONE";
    case kPanLeft:
      return "PAN_LEFT";
    case kPanRight:
      return "PAN_RIGHT";
    case kPanX:
      return "PAN_X";
    case kPanUp:
      return "PAN_UP";
    case kPanLeft | kPanUp:
      return "PAN_LEFT_PAN_UP";
    case kPanRight | kPanUp:
      return "PAN_RIGHT_PAN_UP";
    case kPanX | kPanUp:
      return "PAN_X_PAN_UP";
    case kPanDown:
      return "PAN_DOWN";
    case kPanLeft | kPanDown:
      return "PAN_LEFT_PAN_DOWN";
    case kPanRight | kPanDown:
      return "PAN_RIGHT_PAN_DOWN";
    case kPanX | kPanDown:
      return "PAN_X_PAN_DOWN";
    case kPanY:
      return "PAN_Y";
    case kPanLeft | kPanY:
      return "PAN_LEFT_PAN_Y";
    case kPanRight | kPanY:
      return "PAN_RIGHT_PAN_Y";
    case kAuto:
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
