// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/hit_target.h"

namespace vr {

HitTarget::HitTarget() = default;
HitTarget::~HitTarget() = default;

void HitTarget::Render(UiElementRenderer* renderer,
                       const gfx::Transform& view_proj_matrix) const {}

}  // namespace vr
