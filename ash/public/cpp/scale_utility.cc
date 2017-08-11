// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/scale_utility.h"

#include <cmath>

#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/transform.h"

namespace ash {

float GetScaleFactorForTransform(const gfx::Transform& transform) {
  // We are computing |M * (1, 0) - M * (0, 0)|, which equals
  // abs value of top-left element of M.
  return std::abs(transform.Scale2d().x());
}

}  // namespace ash