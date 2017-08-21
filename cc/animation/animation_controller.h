// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_CONTROLLER_H_
#define CC_ANIMATION_ANIMATION_CONTROLLER_H_

#include <unordered_map>

#include "base/macros.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/effect_controller.h"
#include "cc/trees/element_id.h"

namespace cc {

class AnimationHost;
class AnimationTimeline;
class EffectController;

class CC_ANIMATION_EXPORT AnimationController {
 public:
  explicit AnimationController(int id);
  AnimationController();
  ~AnimationController();

  int id() const { return id_; }

  AnimationHost* animation_host() { return animation_host_; }
  const AnimationHost* animation_host() const { return animation_host_; }
  void SetAnimationHost(AnimationHost* animation_host);

  AnimationTimeline* animation_timeline() { return animation_timeline_; }
  const AnimationTimeline* animation_timeline() const {
    return animation_timeline_;
  }
  void SetAnimationTimeline(AnimationTimeline* timeline);

  void AddEffectController(std::unique_ptr<EffectController>);
  void RemoveEffectController(std::unique_ptr<EffectController>);

  // void AttachElement(ElementId element_id);
  // void DetachElement();

 private:
  // friend class base::RefCounted<AnimationController>;

  //~AnimationController();

  // void SetNeedsCommit();

  // void RegisterPlayer();
  // void UnregisterPlayer();

  // void BindElementAnimations();
  // void UnbindElementAnimations();

  AnimationHost* animation_host_;
  AnimationTimeline* animation_timeline_;

  std::unordered_map<int, std::unique_ptr<EffectController>>
      effect_controllers_;
  int id_;

  DISALLOW_COPY_AND_ASSIGN(AnimationController);
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_CONTROLLER_H_
