// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scroll/ScrollCustomization.h"

namespace blink {

ScrollCustomizationEnabledDirection GetScrollCustomizationForDirection(
    double delta_x,
    double delta_y) {
  return cc::GetScrollCustomizationForDirection(delta_x, delta_y);
}

}  // namespace blink
