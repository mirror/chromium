// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scroll_customization.h"

namespace cc {

ScrollCustomizationEnabledDirection GetScrollCustomizationForDirection(
    double delta_x,
    double delta_y) {
  // TODO(ekaramad, tdresser): Find out the right value for kEpsilon here (see
  // https://crbug.com/510550).
  const double kEpsilon = 0.1f;

  ScrollCustomizationEnabledDirection direction =
      ScrollCustomizationEnabledDirection::kNone;
  if (delta_x > kEpsilon)
    direction |= ScrollCustomizationEnabledDirection::kPanRight;
  if (delta_x < -kEpsilon)
    direction |= ScrollCustomizationEnabledDirection::kPanLeft;
  if (delta_y > kEpsilon)
    direction |= ScrollCustomizationEnabledDirection::kPanDown;
  if (delta_y < -kEpsilon)
    direction |= ScrollCustomizationEnabledDirection::kPanUp;

  return direction;
}

}  // namespace cc
