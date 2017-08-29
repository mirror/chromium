// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_TICKER_H_
#define CC_ANIMATION_ANIMATION_TICKER_H_

#include "base/memory/ref_counted.h"
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

class CC_ANIMATION_EXPORT AnimationTicker {
 public:
  explicit AnimationTicker(AnimationPlayer*);
  ~AnimationTicker();

  // ElementAnimations object where this controller is listed.
  scoped_refptr<ElementAnimations> element_animations() const {
    return element_animations_;
  }

  ElementId element_id() const { return element_id_; }

  void BindElementAnimations(AnimationHost*);
  void UnbindElementAnimations();

  void AttachElement(ElementId);
  void DetachElement();

  void Tick(base::TimeTicks monotonic_time);
  void TickAnimations(base::TimeTicks monotonic_time);

  void AddAnimation(AnimationHost*, std::unique_ptr<Animation>);
  void AnimationAdded();
  void PauseAnimation(int animation_id, double time_offset);
  bool RemoveAnimation(int animation_id);
  void AbortAnimation(int animation_id, base::TimeTicks last_tick_time);
  void AbortAnimations(TargetProperty::Type,
                       bool needs_completion,
                       base::TimeTicks last_tick_time);

  bool NotifyAnimationStarted(const AnimationEvent&);
  bool NotifyAnimationFinished(const AnimationEvent&);
  bool NotifyAnimationAborted(const AnimationEvent&);

  bool HasTickingAnimation() const;
  bool HasNonDeletedAnimation() const;

  void StartAnimationsAndPromoteIfNeeded(base::TimeTicks, AnimationEvents*);
  void StartAnimations(base::TimeTicks);
  void PromoteStartedAnimations(base::TimeTicks, AnimationEvents*);

  void MarkAnimationsForDeletion(base::TimeTicks, AnimationEvents*);

  void MarkFinishedAnimations(base::TimeTicks);
  void ActivateAnimations();

  bool HasTransformAnimationThatInflatesBounds(
      base::TimeTicks last_tick_time) const;
  bool TransformAnimationBoundsForBox(const gfx::BoxF& box,
                                      gfx::BoxF* bounds,
                                      base::TimeTicks last_tick_time) const;

  bool HasOnlyTranslationTransforms(ElementListType) const;

  bool AnimationsPreserveAxisAlignment() const;

  bool AnimationStartScale(ElementListType, float* start_scale) const;
  bool MaximumTargetScale(ElementListType, float* max_scale) const;

  bool IsPotentiallyAnimatingProperty(TargetProperty::Type,
                                      ElementListType) const;
  bool IsCurrentlyAnimatingProperty(TargetProperty::Type,
                                    ElementListType,
                                    base::TimeTicks last_tick_time) const;

  Animation* GetAnimation(TargetProperty::Type) const;
  Animation* GetAnimationById(int animation_id) const;
  void GetPropertyAnimationState(PropertyAnimationState* pending_state,
                                 PropertyAnimationState* active_state,
                                 base::TimeTicks last_tick_time) const;

  void PurgeAnimationsMarkedForDeletion(bool impl_only);

  std::string AnimationsToString() const;

  bool has_any_animation() const { return !animations_.empty(); }

  void MarkAbortedAnimationsForDeletion(AnimationTicker* effect_controller_impl,
                                        base::TimeTicks last_tick_time,
                                        base::TimeTicks impl_last_tick_time);

  gfx::ScrollOffset ScrollOffsetForAnimation() const;
  bool HasElementInActiveList() const;

  void PushNewAnimationsToImplThread(
      AnimationTicker* effect_controller_impl) const;
  void RemoveAnimationsCompletedOnMainThread(
      AnimationTicker* effect_controller_impl) const;
  void PushPropertiesToImplThread(
      AnimationTicker* effect_controller_impl) const;

 private:
  std::vector<std::unique_ptr<Animation>> animations_;
  AnimationPlayer* animation_player_;
  ElementId element_id_;

  // element_animations_ is non-null if controller is attached to an element.
  scoped_refptr<ElementAnimations> element_animations_;

  // Only try to start animations when new animations are added or when the
  // previous attempt at starting animations failed to start all animations.
  bool needs_to_start_animations_;
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_TICKER_H_
