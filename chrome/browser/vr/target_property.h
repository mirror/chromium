// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TARGET_PROPERTY_H_
#define CHROME_BROWSER_VR_TARGET_PROPERTY_H_

namespace vr {

// Must be zero-based as this will be stored in a bitset.
enum TargetProperty {
  TRANSFORM = 0,
  LAYOUT_OFFSET,
  OPACITY,
  BOUNDS,
  BACKGROUND_COLOR,
  FOREGROUND_COLOR,
  GRID_COLOR,
  SPINNER_ANGLE_START,
  SPINNER_ANGLE_SWEEP,
  SPINNER_ROTATION,
  CIRCLE_GROW,

  // These must be contiguous.
  GRADIENT_GROUND_COLOR,
  GRADIENT_MID_GROUND_COLOR,
  GRADIENT_BELOW_HORIZON_COLOR,
  GRADIENT_HORIZON_COLOR,
  GRADIENT_ABOVE_HORIZON_COLOR,
  GRADIENT_MID_SKY_COLOR,
  GRADIENT_SKY_COLOR,
  FIRST_GRADIENT_COLOR = GRADIENT_GROUND_COLOR,
  LAST_GRADIENT_COLOR = GRADIENT_SKY_COLOR,

  // This must be last.
  NUM_TARGET_PROPERTIES
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TARGET_PROPERTY_H_
