// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/fullscreen_window_animation_observer.h"

#include "ui/compositor/layer_animator.h"

namespace ash {

FullScreenWindowAnimationObserver::FullScreenWindowAnimationObserver() =
    default;
FullScreenWindowAnimationObserver::~FullScreenWindowAnimationObserver() =
    default;

void FullScreenWindowAnimationObserver::OnImplicitAnimationsCompleted() {
  for (auto animator_transform : animator_transform_map_)
    animator_transform.first->SetTransform(animator_transform.second);
  animator_transform_map_.clear();
  delete this;
}

void FullScreenWindowAnimationObserver::add_animator_transform_pair(
    scoped_refptr<ui::LayerAnimator> animator,
    gfx::Transform transform) {
  animator_transform_map_[animator] = transform;
}

}  // namespace ash
