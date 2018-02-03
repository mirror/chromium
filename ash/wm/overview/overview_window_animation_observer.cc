// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_window_animation_observer.h"

#include "ui/compositor/layer_animator.h"

namespace ash {

OverviewWindowAnimaitonObserver::OverviewWindowAnimaitonObserver() = default;
OverviewWindowAnimaitonObserver::~OverviewWindowAnimaitonObserver() = default;

void OverviewWindowAnimaitonObserver::OnImplicitAnimationsCompleted() {
  for (auto animator_transform : animator_transform_map_)
    animator_transform.first->SetTransform(animator_transform.second);
  animator_transform_map_.clear();
  delete this;
}

void OverviewWindowAnimaitonObserver::AddAnimatorTransformPair(
    scoped_refptr<ui::LayerAnimator> animator,
    const gfx::Transform& transform) {
  animator_transform_map_[animator] = transform;
}

}  // namespace ash
