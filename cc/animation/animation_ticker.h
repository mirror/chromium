// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_TICKER_H_
#define CC_ANIMATION_ANIMATION_TICKER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/element_animations.h"
#include "cc/trees/element_id.h"
#include "cc/trees/mutator_host_client.h"
#include "cc/trees/target_property.h"
#include "ui/gfx/geometry/box_f.h"
#include "ui/gfx/geometry/scroll_offset.h"

#include <memory>
#include <vector>

namespace cc {

class Animation;
class AnimationHost;
class AnimationPlayer;
struct PropertyAnimationState;

// An AnimationTicker owns a group of Animations for a single target (identified
// by a ElementId). It is responsible for managing the animations' running
// states (starting, running, paused, etc), as well as ticking the animations
// when it is requested to produce new outputs for a given time.
//
// Note that a single AnimationTicker may not own all the animations for a given
// target. AnimationTicker is only a grouping mechanism for related animations.
// The commonality between animations on the same target is found via
// ElementAnimations - there is only one ElementAnimations for a given target.
class CC_ANIMATION_EXPORT AnimationTicker {
 public:
  explicit AnimationTicker(AnimationPlayer*);
  ~AnimationTicker();

  // ElementAnimations object where this controller is listed.
  scoped_refptr<ElementAnimations> element_animations() const {
    return element_animations_;
  }

  bool has_bound_element_animations() const { return !!element_animations_; }

  bool has_attached_element() const { return !!element_id_; }

  ElementId element_id() const { return element_id_; }

  bool has_any_animation() const { return !animations_.empty(); }

  // When a scroll animation is removed on the main thread, its compositor
  // thread counterpart continues producing scroll deltas until activation.
  // These scroll deltas need to be cleared at activation, so that the active
  // element's scroll offset matches the offset provided by the main thread
  // rather than a combination of this offset and scroll deltas produced by the
  // removed animation. This is to provide the illusion of synchronicity to JS
  // that simultaneously removes an animation and sets the scroll offset.
  bool scroll_offset_animation_was_interrupted() const {
    return scroll_offset_animation_was_interrupted_;
  }

  void BindElementAnimations(AnimationHost*);
  void UnbindElementAnimations();

  void AttachElement(ElementId);
  void DetachElement();

  void Tick(base::TimeTicks monotonic_time);
  static void TickAnimation(base::TimeTicks monotonic_time,
                            Animation* animation,
                            AnimationTarget* target);
  void RemoveFromTicking();

  void UpdateState(bool start_ready_animations, AnimationEvents* events);
  void UpdateTickingState(UpdateTickingType type);

  void AddAnimation(std::unique_ptr<Animation>);
  void PauseAnimation(int animation_id, double time_offset);
  void RemoveAnimation(int animation_id);
  void AbortAnimation(int animation_id);
  void AbortAnimations(TargetProperty::Type, bool needs_completion);

  void ActivateAnimations();

  void AnimationAdded();

  bool NotifyAnimationStarted(const AnimationEvent&);
  bool NotifyAnimationFinished(const AnimationEvent&);
  bool NotifyAnimationAborted(const AnimationEvent&);

  bool HasTickingAnimation() const;
  bool HasNonDeletedAnimation() const;

  bool HasOnlyTranslationTransforms(ElementListType) const;

  bool AnimationsPreserveAxisAlignment() const;

  bool AnimationStartScale(ElementListType, float* start_scale) const;
  bool MaximumTargetScale(ElementListType, float* max_scale) const;

  bool IsPotentiallyAnimatingProperty(TargetProperty::Type,
                                      ElementListType) const;
  bool IsCurrentlyAnimatingProperty(TargetProperty::Type,
                                    ElementListType) const;

  Animation* GetAnimation(TargetProperty::Type) const;
  Animation* GetAnimationById(int animation_id) const;

  void GetPropertyAnimationState(PropertyAnimationState* pending_state,
                                 PropertyAnimationState* active_state) const;

  void MarkAbortedAnimationsForDeletion(
      AnimationTicker* effect_controller_impl);
  void PurgeAnimationsMarkedForDeletion(bool impl_only);
  void PushNewAnimationsToImplThread(
      AnimationTicker* effect_controller_impl) const;
  void RemoveAnimationsCompletedOnMainThread(
      AnimationTicker* effect_controller_impl) const;
  void PushPropertiesToImplThread(AnimationTicker* effect_controller_impl);

  std::string AnimationsToString() const;

 private:
  void StartAnimations(base::TimeTicks);
  void PromoteStartedAnimations(AnimationEvents*);

  void MarkAnimationsForDeletion(base::TimeTicks, AnimationEvents*);
  void MarkFinishedAnimations(base::TimeTicks);

  bool HasElementInActiveList() const;
  gfx::ScrollOffset ScrollOffsetForAnimation() const;

  std::vector<std::unique_ptr<Animation>> animations_;
  AnimationPlayer* animation_player_;
  ElementId element_id_;

  // element_animations_ is non-null if controller is attached to an element.
  scoped_refptr<ElementAnimations> element_animations_;

  // Only try to start animations when new animations are added or when the
  // previous attempt at starting animations failed to start all animations.
  bool needs_to_start_animations_;

  bool scroll_offset_animation_was_interrupted_;

  bool is_ticking_;
  base::TimeTicks last_tick_time_;

  DISALLOW_COPY_AND_ASSIGN(AnimationTicker);
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_TICKER_H_
