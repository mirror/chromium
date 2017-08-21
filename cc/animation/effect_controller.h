// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_EFFECT_CONTROLLER_H_
#define CC_ANIMATION_EFFECT_CONTROLLER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_controller.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/element_animations.h"
#include "cc/trees/element_id.h"

namespace cc {

// class AnimationDelegate;
// class AnimationEvents;
// class AnimationHost;
// class AnimationTimeline;
// struct AnimationEvent;
// struct PropertyAnimationState;

class CC_ANIMATION_EXPORT EffectController {
 public:
  static std::unique_ptr<EffectController> Create(int id);
  std::unique_ptr<EffectController> CreateImplInstance() const;
  ~EffectController();

  int id() const { return id_; }

  ElementId element_id() { return element_id_; }
  void SetElementId(ElementId element_id);

  DISALLOW_COPY_AND_ASSIGN(EffectController);

 private:
  explicit EffectController(int id);

  int id_;
  ElementId element_id_;
};

}  // namespace cc

#endif  // CC_ANIMATION_EFFECT_CONTROLLER_H_
