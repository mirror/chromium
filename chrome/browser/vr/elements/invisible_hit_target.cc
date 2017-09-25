// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/invisible_hit_target.h"

namespace vr {

InvisibleHitTarget::InvisibleHitTarget() = default;
InvisibleHitTarget::~InvisibleHitTarget() = default;

void InvisibleHitTarget::Render(UiElementRenderer* renderer,
                                const gfx::Transform& view_proj_matrix) const {}

}  // namespace vr
