// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_OVERVIEW_WINDOW_ANIMATION_OBSERVER_H_
#define ASH_WM_OVERVIEW_OVERVIEW_WINDOW_ANIMATION_OBSERVER_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "ui/compositor/layer_animation_observer.h"

namespace ui {
class LayerAnimator;
}

namespace ash {

// An observer sets transforms of a list of overview windows when the observed
// animation is complete.
class OverviewWindowAnimaitonObserver : public ui::ImplicitAnimationObserver {
 public:
  OverviewWindowAnimaitonObserver();
  ~OverviewWindowAnimaitonObserver() override;

  // ui::ImplicitAnimationObserver:
  void OnImplicitAnimationsCompleted() override;

  void AddAnimatorTransformPair(scoped_refptr<ui::LayerAnimator> animator,
                                const gfx::Transform& transform);

 private:
  // Stores the animators of the windows' layers and corresponding transforms.
  std::map<scoped_refptr<ui::LayerAnimator>, gfx::Transform>
      animator_transform_map_;

  DISALLOW_COPY_AND_ASSIGN(OverviewWindowAnimaitonObserver);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_OVERVIEW_WINDOW_ANIMATION_OBSERVER_H_
