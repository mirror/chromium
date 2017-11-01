// Copyright 2015 The Chromium Authors. All rights reserved.
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

class AnimationDelegate;
class AnimationEvents;
class AnimationHost;
class AnimationTimeline;
class AnimationTicker;
struct AnimationEvent;

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
  int id() const { return id_; }
  ElementId element_id() const;

  // TODO(smcgruer): Only used by a ui/ unittest: remove.
  bool has_any_animation() const;

  scoped_refptr<ElementAnimations> element_animations() const;
  void AttachElement(ElementId element_id);
  void DetachElement();
  void AddAnimation(std::unique_ptr<Animation> animation);
  void PauseAnimation(int animation_id, double time_offset);
  void RemoveAnimation(int animation_id);
  void AbortAnimation(int animation_id);
  void AbortAnimations(TargetProperty::Type target_property,
                       bool needs_completion);

  // void PushPropertiesTo(AnimationPlayer* player_impl) override;

  void UpdateState(bool start_ready_animations, AnimationEvents* events);
  void Tick(base::TimeTicks monotonic_time) override;

  //  void SetNeedsPushProperties();

  // Make animations affect active elements if and only if they affect
  // pending elements. Any animations that no longer affect any elements
  // are deleted.
  void ActivateAnimations();

  // Returns the animation animating the given property that is either
  // running, or is next to run, if such an animation exists.
  Animation* GetAnimation(TargetProperty::Type target_property) const;
  std::string ToString() const;

 private:
  friend class base::RefCounted<SingleAnimationPlayer>;

  void RegisterPlayer();
  void UnregisterPlayer();
  int id_;

 protected:
  explicit SingleAnimationPlayer(int id);
  ~SingleAnimationPlayer() override;

  DISALLOW_COPY_AND_ASSIGN(SingleAnimationPlayer);
};

}  // namespace cc

#endif  // CC_ANIMATION_SINGLE_ANIMATION_PLAYER_H_
