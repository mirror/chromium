// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_EFFECT_CONTROLLER_H_
#define CC_ANIMATION_EFFECT_CONTROLLER_H_

#include "cc/animation/animation_events.h"
#include "cc/animation/animation_export.h"
#include "cc/trees/element_id.h"
#include "cc/trees/mutator_host_client.h"
#include "cc/trees/target_property.h"
#include "ui/gfx/geometry/box_f.h"

#include <memory>
#include <vector>

namespace cc {

class Animation;
struct PropertyAnimationState;

class CC_ANIMATION_EXPORT EffectController {
 public:
  EffectController();
  ~EffectController();

  void AddAnimation(std::unique_ptr<Animation>);
  void PauseAnimation(int animation_id, double time_offset);
  bool RemoveAnimation(int animation_id);
  bool AbortAnimations(TargetProperty::Type,
                       bool needs_completion,
                       base::TimeTicks last_tick_time);

  bool NotifyAnimationStarted(const AnimationEvent&);
  bool NotifyAnimationFinished(const AnimationEvent&);
  bool NotifyAnimationAborted(const AnimationEvent&);

  bool HasTickingAnimation() const;
  bool HasNonDeletedAnimation() const;

  void StartAnimations(base::TimeTicks);
  void PromoteStartedAnimations(ElementId, base::TimeTicks, AnimationEvents*);

  bool MarkAnimationsForDeletion(ElementId, base::TimeTicks, AnimationEvents*);

  bool MarkFinishedAnimations(base::TimeTicks);
  bool ActivateAnimations();

  bool TransformAnimationBoundsForBox(const gfx::BoxF& box,
                                      gfx::BoxF* bounds) const;

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

  // TODO(smcgruer): Make private
  std::vector<std::unique_ptr<Animation>> animations_;
  // private:
};

}  // namespace cc

#endif  // CC_ANIMATION_EFFECT_CONTROLLER_H_
