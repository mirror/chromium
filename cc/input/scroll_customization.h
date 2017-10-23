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
  kAuto = kPanX | kPanY
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

// Returns the scroll customization which is compatible with the provided
// direction deltas.
CC_EXPORT ScrollCustomizationEnabledDirection
GetScrollCustomizationForDirection(double delta_x, double delta_y);

}  // namespace cc

#endif  // CC_INPUT_SCROLL_CUSTOMIZATION_H_
