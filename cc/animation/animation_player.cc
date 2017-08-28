// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_player.h"

#include <inttypes.h>
#include <algorithm>

#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "cc/animation/animation_delegate.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_timeline.h"
#include "cc/animation/effect_controller.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/animation/transform_operations.h"
#include "cc/trees/property_animation_state.h"

namespace cc {

scoped_refptr<AnimationPlayer> AnimationPlayer::Create(int id) {
  return make_scoped_refptr(new AnimationPlayer(id));
}

AnimationPlayer::AnimationPlayer(int id)
    : animation_host_(),
      animation_timeline_(),
      element_animations_(),
      animation_delegate_(),
      id_(id),
      needs_push_properties_(false),
      needs_to_start_animations_(false),
      is_ticking_(false),
      scroll_offset_animation_was_interrupted_(false),
      effect_controller_(new EffectController) {
  DCHECK(id_);
}

AnimationPlayer::~AnimationPlayer() {
  DCHECK(!animation_timeline_);
  DCHECK(!element_animations_);
}

scoped_refptr<AnimationPlayer> AnimationPlayer::CreateImplInstance() const {
  scoped_refptr<AnimationPlayer> player = AnimationPlayer::Create(id());
  return player;
}

void AnimationPlayer::SetAnimationHost(AnimationHost* animation_host) {
  animation_host_ = animation_host;
}

void AnimationPlayer::SetAnimationTimeline(AnimationTimeline* timeline) {
  if (animation_timeline_ == timeline)
    return;

  // We need to unregister player to manage ElementAnimations and observers
  // properly.
  if (element_id_ && element_animations_)
    UnregisterPlayer();

  animation_timeline_ = timeline;

  // Register player only if layer AND host attached.
  if (element_id_ && animation_host_)
    RegisterPlayer();
}

void AnimationPlayer::AttachElement(ElementId element_id) {
  DCHECK(!element_id_);
  DCHECK(element_id);

  element_id_ = element_id;

  // Register player only if layer AND host attached.
  if (animation_host_)
    RegisterPlayer();
}

void AnimationPlayer::DetachElement() {
  DCHECK(element_id_);

  if (animation_host_)
    UnregisterPlayer();

  element_id_ = ElementId();
}

void AnimationPlayer::RegisterPlayer() {
  DCHECK(element_id_);
  DCHECK(animation_host_);
  DCHECK(!element_animations_);

  // Create ElementAnimations or re-use existing.
  animation_host_->RegisterPlayerForElement(element_id_, this);
  // Get local reference to shared ElementAnimations.
  BindElementAnimations();
}

void AnimationPlayer::UnregisterPlayer() {
  DCHECK(element_id_);
  DCHECK(animation_host_);
  DCHECK(element_animations_);

  UnbindElementAnimations();
  // Destroy ElementAnimations or release it if it's still needed.
  animation_host_->UnregisterPlayerForElement(element_id_, this);
}

void AnimationPlayer::BindElementAnimations() {
  DCHECK(!element_animations_);
  element_animations_ =
      animation_host_->GetElementAnimationsForElementId(element_id_);
  DCHECK(element_animations_);

  if (has_any_animation())
    AnimationAdded();

  SetNeedsPushProperties();
}

void AnimationPlayer::UnbindElementAnimations() {
  SetNeedsPushProperties();
  element_animations_ = nullptr;
}

void AnimationPlayer::AddAnimation(std::unique_ptr<Animation> animation) {
  // TODO(smcgruer): Move DCHECKs into effect controller?
  DCHECK(animation->target_property_id() != TargetProperty::SCROLL_OFFSET ||
         (animation_host_ && animation_host_->SupportsScrollAnimations()));
  DCHECK(!animation->is_impl_only() ||
         animation->target_property_id() == TargetProperty::SCROLL_OFFSET);

  effect_controller_->AddAnimation(std::move(animation));
  if (element_animations_) {
    AnimationAdded();
    SetNeedsPushProperties();
  }
}

void AnimationPlayer::AnimationAdded() {
  DCHECK(element_animations_);

  SetNeedsCommit();
  needs_to_start_animations_ = true;

  UpdateTickingState(UpdateTickingType::NORMAL);
  element_animations_->UpdateClientAnimationState();
}

void AnimationPlayer::PauseAnimation(int animation_id, double time_offset) {
  effect_controller_->PauseAnimation(animation_id, time_offset);
  if (element_animations_) {
    SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationPlayer::RemoveAnimation(int animation_id) {
  bool animation_removed = effect_controller_->RemoveAnimation(animation_id);
  if (element_animations_) {
    UpdateTickingState(UpdateTickingType::NORMAL);
    if (animation_removed)
      element_animations_->UpdateClientAnimationState();
    SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationPlayer::AbortAnimation(int animation_id) {
  if (Animation* animation = GetAnimationById(animation_id)) {
    if (!animation->is_finished()) {
      animation->SetRunState(Animation::ABORTED, last_tick_time_);
      if (element_animations_)
        element_animations_->UpdateClientAnimationState();
    }
  }

  if (element_animations_) {
    SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationPlayer::AbortAnimations(TargetProperty::Type target_property,
                                      bool needs_completion) {
  bool aborted_animation = effect_controller_->AbortAnimations(
      target_property, needs_completion, last_tick_time_);
  if (element_animations_) {
    if (aborted_animation)
      element_animations_->UpdateClientAnimationState();
    SetNeedsCommit();
    SetNeedsPushProperties();
  }
}

void AnimationPlayer::PushPropertiesTo(AnimationPlayer* player_impl) {
  if (!needs_push_properties_)
    return;
  needs_push_properties_ = false;

  // Create or destroy ElementAnimations.
  if (element_id_ != player_impl->element_id()) {
    if (player_impl->element_id())
      player_impl->DetachElement();
    if (element_id_)
      player_impl->AttachElement(element_id_);
  }

  if (!has_any_animation() && !player_impl->has_any_animation())
    return;

  MarkAbortedAnimationsForDeletion(player_impl);
  PurgeAnimationsMarkedForDeletion(/* impl_only */ false);
  PushNewAnimationsToImplThread(player_impl);

  // Remove finished impl side animations only after pushing,
  // and only after the animations are deleted on the main thread
  // this insures we will never push an animation twice.
  RemoveAnimationsCompletedOnMainThread(player_impl);

  PushPropertiesToImplThread(player_impl);

  player_impl->UpdateTickingState(UpdateTickingType::NORMAL);
}

void AnimationPlayer::Tick(base::TimeTicks monotonic_time) {
  DCHECK(!monotonic_time.is_null());
  DCHECK(element_animations_);

  if (!element_animations_->has_element_in_any_list())
    return;

  if (needs_to_start_animations())
    StartAnimations(monotonic_time);

  TickAnimations(monotonic_time);

  last_tick_time_ = monotonic_time;
  element_animations_->UpdateClientAnimationState();
}

void AnimationPlayer::UpdateState(bool start_ready_animations,
                                  AnimationEvents* events) {
  DCHECK(element_animations_);
  if (!element_animations_->has_element_in_active_list())
    return;

  // Animate hasn't been called, this happens if an element has been added
  // between the Commit and Draw phases.
  if (last_tick_time_ == base::TimeTicks())
    return;

  if (start_ready_animations)
    PromoteStartedAnimations(last_tick_time_, events);

  MarkFinishedAnimations(last_tick_time_);
  MarkAnimationsForDeletion(last_tick_time_, events);
  PurgeAnimationsMarkedForDeletion(/* impl_only */ true);

  if (start_ready_animations) {
    if (needs_to_start_animations()) {
      StartAnimations(last_tick_time_);
      PromoteStartedAnimations(last_tick_time_, events);
    }
  }

  UpdateTickingState(UpdateTickingType::NORMAL);
}

void AnimationPlayer::UpdateTickingState(UpdateTickingType type) {
  bool force = type == UpdateTickingType::FORCE;
  if (animation_host_) {
    bool was_ticking = is_ticking_;
    is_ticking_ = HasNonDeletedAnimation();

    bool has_element_in_any_list =
        element_animations_->has_element_in_any_list();

    if (is_ticking_ && ((!was_ticking && has_element_in_any_list) || force)) {
      animation_host_->AddToTicking(this);
    } else if (!is_ticking_ && (was_ticking || force)) {
      RemoveFromTicking();
    }
  }
}

void AnimationPlayer::RemoveFromTicking() {
  DCHECK(animation_host_);
  // Resetting last_tick_time_ here ensures that calling ::UpdateState
  // before ::Animate doesn't start an animation.
  is_ticking_ = false;
  last_tick_time_ = base::TimeTicks();
  animation_host_->RemoveFromTicking(this);
}

bool AnimationPlayer::NotifyAnimationStarted(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);

  if (effect_controller_->NotifyAnimationStarted(event)) {
    if (animation_delegate_) {
      animation_delegate_->NotifyAnimationStarted(
          event.monotonic_time, event.target_property, event.group_id);
    }
    return true;
  }
  return false;
}

bool AnimationPlayer::NotifyAnimationFinished(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  if (effect_controller_->NotifyAnimationFinished(event)) {
    if (animation_delegate_) {
      animation_delegate_->NotifyAnimationFinished(
          event.monotonic_time, event.target_property, event.group_id);
    }
    return true;
  }

  // This is for the case when an animation is already removed on main thread,
  // but the impl version of it sent a finished event and is now waiting for
  // deletion. We would need to delete that animation during push properties.
  SetNeedsPushProperties();
  return false;
}

bool AnimationPlayer::NotifyAnimationFinishedForTesting(
    TargetProperty::Type target_property,
    int group_id) {
  AnimationEvent event(AnimationEvent::FINISHED, element_id_, group_id,
                       target_property, base::TimeTicks());
  return NotifyAnimationFinished(event);
}

bool AnimationPlayer::NotifyAnimationAborted(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  if (effect_controller_->NotifyAnimationAborted(event)) {
    if (animation_delegate_) {
      animation_delegate_->NotifyAnimationAborted(
          event.monotonic_time, event.target_property, event.group_id);
    }
    return true;
  }
  return false;
}

void AnimationPlayer::NotifyAnimationTakeover(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  DCHECK(event.target_property == TargetProperty::SCROLL_OFFSET);

  // We need to purge animations marked for deletion on CT.
  SetNeedsPushProperties();

  if (animation_delegate_) {
    DCHECK(event.curve);
    std::unique_ptr<AnimationCurve> animation_curve = event.curve->Clone();
    animation_delegate_->NotifyAnimationTakeover(
        event.monotonic_time, event.target_property, event.animation_start_time,
        std::move(animation_curve));
  }
}

void AnimationPlayer::SetNeedsCommit() {
  DCHECK(animation_host_);
  animation_host_->SetNeedsCommit();
}

void AnimationPlayer::SetNeedsPushProperties() {
  needs_push_properties_ = true;

  DCHECK(animation_timeline_);
  animation_timeline_->SetNeedsPushProperties();

  DCHECK(element_animations_);
  element_animations_->SetNeedsPushProperties();
}

bool AnimationPlayer::HasTickingAnimation() const {
  return effect_controller_->HasTickingAnimation();
}

bool AnimationPlayer::has_any_animation() const {
  return effect_controller_->has_any_animation();
}

bool AnimationPlayer::HasNonDeletedAnimation() const {
  return effect_controller_->HasNonDeletedAnimation();
}

void AnimationPlayer::StartAnimations(base::TimeTicks monotonic_time) {
  DCHECK(needs_to_start_animations_);
  needs_to_start_animations_ = false;
  effect_controller_->StartAnimations(monotonic_time);
}

void AnimationPlayer::PromoteStartedAnimations(base::TimeTicks monotonic_time,
                                               AnimationEvents* events) {
  effect_controller_->PromoteStartedAnimations(element_id_, monotonic_time,
                                               events);
}

void AnimationPlayer::MarkAnimationsForDeletion(base::TimeTicks monotonic_time,
                                                AnimationEvents* events) {
  // Notify about animations waiting for deletion.
  // We need to purge animations marked for deletion, which happens in
  // PushProperties().
  if (effect_controller_->MarkAnimationsForDeletion(element_id_, monotonic_time,
                                                    events))
    SetNeedsPushProperties();
}

void AnimationPlayer::TickAnimation(base::TimeTicks monotonic_time,
                                    Animation* animation,
                                    AnimationTarget* target) {
  if ((animation->run_state() != Animation::STARTING &&
       animation->run_state() != Animation::RUNNING &&
       animation->run_state() != Animation::PAUSED) ||
      !animation->InEffect(monotonic_time)) {
    return;
  }

  AnimationCurve* curve = animation->curve();
  base::TimeDelta trimmed =
      animation->TrimTimeToCurrentIteration(monotonic_time);

  switch (curve->Type()) {
    case AnimationCurve::TRANSFORM:
      target->NotifyClientTransformOperationsAnimated(
          curve->ToTransformAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::FLOAT:
      target->NotifyClientFloatAnimated(
          curve->ToFloatAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::FILTER:
      target->NotifyClientFilterAnimated(
          curve->ToFilterAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::COLOR:
      target->NotifyClientColorAnimated(
          curve->ToColorAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::SCROLL_OFFSET:
      target->NotifyClientScrollOffsetAnimated(
          curve->ToScrollOffsetAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::SIZE:
      target->NotifyClientSizeAnimated(
          curve->ToSizeAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
    case AnimationCurve::BOOLEAN:
      target->NotifyClientBooleanAnimated(
          curve->ToBooleanAnimationCurve()->GetValue(trimmed),
          animation->target_property_id(), animation);
      break;
  }
}

void AnimationPlayer::TickAnimations(base::TimeTicks monotonic_time) {
  // TODO(smcgruer): Port this once element_animations_ is in effect controller.
  DCHECK(element_animations_);
  for (auto& animation : effect_controller_->animations_)
    TickAnimation(monotonic_time, animation.get(), element_animations_.get());
  last_tick_time_ = monotonic_time;
}

void AnimationPlayer::MarkFinishedAnimations(base::TimeTicks monotonic_time) {
  DCHECK(element_animations_);
  if (effect_controller_->MarkFinishedAnimations(monotonic_time))
    element_animations_->UpdateClientAnimationState();
}

void AnimationPlayer::ActivateAnimations() {
  if (effect_controller_->ActivateAnimations())
    element_animations_->UpdateClientAnimationState();

  scroll_offset_animation_was_interrupted_ = false;
  UpdateTickingState(UpdateTickingType::NORMAL);
}

bool AnimationPlayer::HasTransformAnimationThatInflatesBounds() const {
  return IsCurrentlyAnimatingProperty(TargetProperty::TRANSFORM,
                                      ElementListType::ACTIVE) ||
         IsCurrentlyAnimatingProperty(TargetProperty::TRANSFORM,
                                      ElementListType::PENDING);
}

bool AnimationPlayer::TransformAnimationBoundsForBox(const gfx::BoxF& box,
                                                     gfx::BoxF* bounds) const {
  // TODO(smcgruer): Move check into effect controller.
  DCHECK(HasTransformAnimationThatInflatesBounds())
      << "TransformAnimationBoundsForBox will give incorrect results if there "
      << "are no transform animations affecting bounds, non-animated transform "
      << "is not known";

  return effect_controller_->TransformAnimationBoundsForBox(box, bounds);
}

bool AnimationPlayer::HasOnlyTranslationTransforms(
    ElementListType list_type) const {
  return effect_controller_->HasOnlyTranslationTransforms(list_type);
}

bool AnimationPlayer::AnimationsPreserveAxisAlignment() const {
  return effect_controller_->AnimationsPreserveAxisAlignment();
}

bool AnimationPlayer::AnimationStartScale(ElementListType list_type,
                                          float* start_scale) const {
  return effect_controller_->AnimationStartScale(list_type, start_scale);
}

bool AnimationPlayer::MaximumTargetScale(ElementListType list_type,
                                         float* max_scale) const {
  return effect_controller_->MaximumTargetScale(list_type, max_scale);
}

bool AnimationPlayer::IsPotentiallyAnimatingProperty(
    TargetProperty::Type target_property,
    ElementListType list_type) const {
  return effect_controller_->IsPotentiallyAnimatingProperty(target_property,
                                                            list_type);
}

bool AnimationPlayer::IsCurrentlyAnimatingProperty(
    TargetProperty::Type target_property,
    ElementListType list_type) const {
  return effect_controller_->IsCurrentlyAnimatingProperty(
      target_property, list_type, last_tick_time_);
}

bool AnimationPlayer::HasElementInActiveList() const {
  DCHECK(element_animations_);
  return element_animations_->has_element_in_active_list();
}

gfx::ScrollOffset AnimationPlayer::ScrollOffsetForAnimation() const {
  DCHECK(element_animations_);
  return element_animations_->ScrollOffsetForAnimation();
}

Animation* AnimationPlayer::GetAnimation(
    TargetProperty::Type target_property) const {
  return effect_controller_->GetAnimation(target_property);
}

Animation* AnimationPlayer::GetAnimationById(int animation_id) const {
  return effect_controller_->GetAnimationById(animation_id);
}

void AnimationPlayer::GetPropertyAnimationState(
    PropertyAnimationState* pending_state,
    PropertyAnimationState* active_state) const {
  effect_controller_->GetPropertyAnimationState(pending_state, active_state,
                                                last_tick_time_);
}

void AnimationPlayer::MarkAbortedAnimationsForDeletion(
    AnimationPlayer* animation_player_impl) const {
  bool animation_aborted = false;

  // TODO(smcgruer): Port this.
  auto& animations_impl =
      animation_player_impl->effect_controller_->animations_;
  for (const auto& animation_impl : animations_impl) {
    // If the animation has been aborted on the main thread, mark it for
    // deletion.
    if (Animation* animation = GetAnimationById(animation_impl->id())) {
      if (animation->run_state() == Animation::ABORTED) {
        animation_impl->SetRunState(Animation::WAITING_FOR_DELETION,
                                    animation_player_impl->last_tick_time_);
        animation->SetRunState(Animation::WAITING_FOR_DELETION,
                               last_tick_time_);
        animation_aborted = true;
      }
    }
  }

  if (element_animations_ && animation_aborted)
    element_animations_->SetNeedsUpdateImplClientState();
}

void AnimationPlayer::PurgeAnimationsMarkedForDeletion(bool impl_only) {
  effect_controller_->PurgeAnimationsMarkedForDeletion(impl_only);
}

void AnimationPlayer::PushNewAnimationsToImplThread(
    AnimationPlayer* animation_player_impl) const {
  // Any new animations owned by the main thread's AnimationPlayer are cloned
  // and added to the impl thread's AnimationPlayer.
  // TODO(smcgruer): Port.
  for (size_t i = 0; i < effect_controller_->animations_.size(); ++i) {
    const auto& anim = effect_controller_->animations_[i];

    // If the animation is already running on the impl thread, there is no
    // need to copy it over.
    if (animation_player_impl->GetAnimationById(anim->id()))
      continue;

    if (anim->target_property_id() == TargetProperty::SCROLL_OFFSET &&
        !anim->curve()->ToScrollOffsetAnimationCurve()->HasSetInitialValue()) {
      gfx::ScrollOffset current_scroll_offset;
      if (animation_player_impl->HasElementInActiveList()) {
        current_scroll_offset =
            animation_player_impl->ScrollOffsetForAnimation();
      } else {
        // The owning layer isn't yet in the active tree, so the main thread
        // scroll offset will be up to date.
        current_scroll_offset = ScrollOffsetForAnimation();
      }
      anim->curve()->ToScrollOffsetAnimationCurve()->SetInitialValue(
          current_scroll_offset);
    }

    // The new animation should be set to run as soon as possible.
    Animation::RunState initial_run_state =
        Animation::WAITING_FOR_TARGET_AVAILABILITY;
    std::unique_ptr<Animation> to_add(
        anim->CloneAndInitialize(initial_run_state));
    DCHECK(!to_add->needs_synchronized_start_time());
    to_add->set_affects_active_elements(false);
    animation_player_impl->AddAnimation(std::move(to_add));
  }
}

static bool IsCompleted(Animation* animation,
                        const AnimationPlayer* main_thread_player) {
  if (animation->is_impl_only()) {
    return (animation->run_state() == Animation::WAITING_FOR_DELETION);
  } else {
    Animation* main_thread_animation =
        main_thread_player->GetAnimationById(animation->id());
    return !main_thread_animation || main_thread_animation->is_finished();
  }
}

void AnimationPlayer::RemoveAnimationsCompletedOnMainThread(
    AnimationPlayer* animation_player_impl) const {
  bool animation_completed = false;

  // Animations removed on the main thread should no longer affect pending
  // elements, and should stop affecting active elements after the next call
  // to ActivateAnimations. If already WAITING_FOR_DELETION, they can be removed
  // immediately.
  // TODO(smcgruer): Port.
  auto& animations = animation_player_impl->effect_controller_->animations_;
  for (const auto& animation : animations) {
    if (IsCompleted(animation.get(), this)) {
      animation->set_affects_pending_elements(false);
      animation_completed = true;
    }
  }
  auto affects_active_only_and_is_waiting_for_deletion =
      [](const std::unique_ptr<Animation>& animation) {
        return animation->run_state() == Animation::WAITING_FOR_DELETION &&
               !animation->affects_pending_elements();
      };
  base::EraseIf(animations, affects_active_only_and_is_waiting_for_deletion);

  if (element_animations_ && animation_completed)
    element_animations_->SetNeedsUpdateImplClientState();
}

void AnimationPlayer::PushPropertiesToImplThread(
    AnimationPlayer* animation_player_impl) {
  // TODO(smcgruer): Port.
  for (size_t i = 0; i < effect_controller_->animations_.size(); ++i) {
    Animation* current_impl = animation_player_impl->GetAnimationById(
        effect_controller_->animations_[i]->id());
    if (current_impl)
      effect_controller_->animations_[i]->PushPropertiesTo(current_impl);
  }

  animation_player_impl->scroll_offset_animation_was_interrupted_ =
      scroll_offset_animation_was_interrupted_;
  scroll_offset_animation_was_interrupted_ = false;
}

std::string AnimationPlayer::ToString() const {
  return base::StringPrintf(
      "AnimationPlayer{id=%d, element_id=%s, animations=[%s]}", id_,
      element_id_.ToString().c_str(),
      effect_controller_->AnimationsToString().c_str());
}

}  // namespace cc
