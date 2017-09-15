// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_DRAW_PHASE_H_
#define CHROME_BROWSER_VR_ELEMENTS_DRAW_PHASE_H_

namespace vr {

// Each draw phase is rendered independently in the order specified below.
enum DrawPhase : int {
  // kPhaseNone is to be used for elements that do not draw. Eg, layouts.
  kPhaseBackground = 0,
  kPhaseFloorCeiling,
  kPhaseForeground,
  kPhaseOverlayBackground,
  kPhaseOverlayForeground,
  kPhaseNone,
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_DRAW_PHASE_H_
