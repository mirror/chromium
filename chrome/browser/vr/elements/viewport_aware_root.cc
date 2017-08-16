// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/viewport_aware_root.h"

#include <cmath>

namespace vr {

// static
const float ViewportAwareRoot::kViewportRotationTriggerDegrees = 55.0f;

ViewportAwareRoot::ViewportAwareRoot() {
  set_viewport_aware(true);
}

ViewportAwareRoot::~ViewportAwareRoot() = default;

void ViewportAwareRoot::AdjustPosition(const gfx::Vector3dF& look_at) {
  // This must be a top level element.
  DCHECK(!parent() && viewport_aware());

  gfx::Vector3dF rotated_center_vector{0.f, 0.f, -1.0f};
  viewport_aware_rotation_.TransformVector(&rotated_center_vector);
  gfx::Vector3dF top_projected_look_at{look_at.x(), 0.f, look_at.z()};
  float degrees = gfx::ClockwiseAngleBetweenVectorsInDegrees(
      top_projected_look_at, rotated_center_vector, {0.f, 1.0f, 0.f});
  if (degrees > kViewportRotationTriggerDegrees &&
      degrees < 360.0f - kViewportRotationTriggerDegrees) {
    viewport_aware_rotation_.RotateAboutYAxis(degrees);
  }
}

void ViewportAwareRoot::LayOutChildren() {}

gfx::Transform ViewportAwareRoot::LocalTransform() const {
  return viewport_aware_rotation_;
}

}  // namespace vr
