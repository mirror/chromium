// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_OVERVIEW_FULLSCREEN_WINDOW_ANIMATION_OBSERVER_H_
#define ASH_WM_OVERVIEW_FULLSCREEN_WINDOW_ANIMATION_OBSERVER_H_

#include <map>
#include <memory>
#include <vector>

#include "ui/compositor/layer_animation_observer.h"

namespace ui {
class LayerAnimator;
}

namespace ash {

class FullScreenWindowAnimationObserver : public ui::ImplicitAnimationObserver {
 public:
  FullScreenWindowAnimationObserver();
  ~FullScreenWindowAnimationObserver() override;

  // ui::ImplicitAnimationObserver:
  void OnImplicitAnimationsCompleted() override;

  void add_animator_transform_pair(scoped_refptr<ui::LayerAnimator> animator,
                                   gfx::Transform transform);

 private:
  std::map<scoped_refptr<ui::LayerAnimator>, gfx::Transform>
      animator_transform_map_;

  DISALLOW_COPY_AND_ASSIGN(FullScreenWindowAnimationObserver);
};

}  // namespace ash

#endif  // ASH_WM_OVERVIEW_FULLSCREEN_WINDOW_ANIMATION_OBSERVER_H_
