// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scroll_customization.h"

namespace cc {

ScrollCustomization GetScrollCustomizationForDirection(double delta_x,
                                                       double delta_y) {
  // TODO(ekaramad, tdresser): Find out the right value for kEpsilon here (see
  // https://crbug.com/510550).
  const double kEpsilon = 0.1f;

  ScrollCustomization direction = ScrollCustomization::kScrollCustomizationNone;
  if (delta_x > kEpsilon)
    direction |= ScrollCustomization::kScrollCustomizationPanRight;
  if (delta_x < -kEpsilon)
    direction |= ScrollCustomization::kScrollCustomizationPanLeft;
  if (delta_y > kEpsilon)
    direction |= ScrollCustomization::kScrollCustomizationPanDown;
  if (delta_y < -kEpsilon)
    direction |= ScrollCustomization::kScrollCustomizationPanUp;

  return direction;
}

}  // namespace cc
