// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_SINGLE_ANIMATION_PLAYER_H_
#define CC_ANIMATION_SINGLE_ANIMATION_PLAYER_H_

#include <vector>

#include <memory>
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/animation_player.h"
#include "cc/animation/element_animations.h"
#include "cc/trees/element_id.h"

namespace cc {

class AnimationHost;
class AnimationTimeline;
class AnimationTicker;

// An AnimationPlayer manages grouped sets of animations (each set of which are
// stored in an AnimationTicker), and handles the interaction with the
// AnimationHost and AnimationTimeline.
//
// This class is a CC counterpart for blink::Animation, currently in a 1:1
// relationship. Currently the blink logic is responsible for handling of
// conflicting same-property animations.
//
// Each cc AnimationPlayer has a copy on the impl thread, and will take care of
// synchronizing properties to/from the impl thread when requested.
//
// NOTE(smcgruer): As of 2017/09/06 there is a 1:1 relationship between
// AnimationPlayer and the AnimationTicker. This is intended to become a 1:N
// relationship to allow for grouped animations.
class CC_ANIMATION_EXPORT SingleAnimationPlayer : public AnimationPlayer {
  // public base::RefCounted<SingleAnimationPlayer> {
 public:
  static scoped_refptr<SingleAnimationPlayer> Create(int id);
  scoped_refptr<AnimationPlayer> CreateImplInstance() const override;
  ElementId element_id() const override;

  void SetAnimationTimeline(AnimationTimeline* timeline) override;

  // TODO(smcgruer): Only used by a ui/ unittest: remove.
  bool has_any_animation() const;

  scoped_refptr<ElementAnimations> element_animations() const override;
  void AttachElement(ElementId element_id) override;
  void DetachElement() override;
  void AddAnimation(std::unique_ptr<Animation> animation) override;
  void PauseAnimation(int animation_id, double time_offset) override;
  void RemoveAnimation(int animation_id) override;
  void AbortAnimation(int animation_id) override;
  void AbortAnimations(TargetProperty::Type target_property,
                       bool needs_completion) override;

  // void PushPropertiesTo(AnimationPlayer* player_impl) override;

  void UpdateState(bool start_ready_animations,
                   AnimationEvents* events) override;
  void Tick(base::TimeTicks monotonic_time) override;

  //  void SetNeedsPushProperties();

  // Make animations affect active elements if and only if they affect
  // pending elements. Any animations that no longer affect any elements
  // are deleted.
  void ActivateAnimations() override;

  // Returns the animation animating the given property that is either
  // running, or is next to run, if such an animation exists.
  Animation* GetAnimation(TargetProperty::Type target_property) const override;
  std::string ToString() const override;

 private:
  friend class base::RefCounted<SingleAnimationPlayer>;

  void RegisterPlayer();
  void UnregisterPlayer();

 protected:
  explicit SingleAnimationPlayer(int id);
  ~SingleAnimationPlayer() override;

  DISALLOW_COPY_AND_ASSIGN(SingleAnimationPlayer);
};

}  // namespace cc

#endif  // CC_ANIMATION_SINGLE_ANIMATION_PLAYER_H_
