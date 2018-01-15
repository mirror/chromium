// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scroll/ScrollCustomization.h"

namespace blink {
namespace scroll_customization {

const ScrollDirection kScrollDirectionNone = 0;
const ScrollDirection kScrollDirectionPanLeft = 1 << 0;
const ScrollDirection kScrollDirectionPanRight = 1 << 1;
const ScrollDirection kScrollDirectionPanX =
    kScrollDirectionPanLeft | kScrollDirectionPanRight;
const ScrollDirection kScrollDirectionPanUp = 1 << 2;
const ScrollDirection kScrollDirectionPanDown = 1 << 3;
const ScrollDirection kScrollDirectionPanY =
    kScrollDirectionPanUp | kScrollDirectionPanDown;
const ScrollDirection kScrollDirectionAuto =
    kScrollDirectionPanX | kScrollDirectionPanY;

ScrollDirection GetScrollDirectionFromDeltas(double delta_x, double delta_y) {
  // TODO(ekaramad, tdresser): Find out the right value for kEpsilon here (see
  // https://crbug.com/510550).
  const double kEpsilon = 0.1f;

  ScrollDirection direction = kScrollDirectionNone;
  if (delta_x > kEpsilon)
    direction |= kScrollDirectionPanRight;
  if (delta_x < -kEpsilon)
    direction |= kScrollDirectionPanLeft;
  if (delta_y > kEpsilon)
    direction |= kScrollDirectionPanDown;
  if (delta_y < -kEpsilon)
    direction |= kScrollDirectionPanUp;

  if (!direction) {
    // TODO(ekaramad, sahel): Remove this and perhaps replace with a DCHECK when
    // issue https://crbug.com/728214 is fixed.
    return kScrollDirectionAuto;
  }

  return direction;
  ;
}

}  // namespace scroll_customization
}  // namespace blink
