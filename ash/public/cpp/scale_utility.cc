// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/scale_utility.h"

#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/transform.h"

namespace ash {

float GetScaleFactorForTransform(const gfx::Transform& transform) {
  gfx::Point3F p1(0, 0, 0);
  gfx::Point3F p2(1, 0, 0);
  transform.TransformPoint(&p1);
  transform.TransformPoint(&p2);
  return (p2 - p1).Length();
}

}  // namespace ash