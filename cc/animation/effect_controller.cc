// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/effect_controller.h"

#include "base/stl_util.h"
#include "base/time/time.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/trees/property_animation_state.h"

namespace cc {

EffectController::EffectController() {}

EffectController::~EffectController() {}

void EffectController::AddAnimation(std::unique_ptr<Animation> animation) {
  // DCHECK(animation->target_property_id() != TargetProperty::SCROLL_OFFSET ||
  //       (animation_host_ && animation_host_->SupportsScrollAnimations()));
  // DCHECK(!animation->is_impl_only() ||
  //       animation->target_property_id() == TargetProperty::SCROLL_OFFSET);

  animations_.push_back(std::move(animation));
}

void EffectController::PauseAnimation(int animation_id, double time_offset) {
  const base::TimeDelta time_delta = base::TimeDelta::FromSecondsD(time_offset);
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->id() == animation_id) {
      animations_[i]->SetRunState(Animation::PAUSED,
                                  time_delta + animations_[i]->start_time() +
                                      animations_[i]->time_offset());
    }
  }
}

bool EffectController::RemoveAnimation(int animation_id) {
  bool animation_removed = false;

  // Since we want to use the animations that we're going to remove, we need to
  // use a stable_parition here instead of remove_if. Remove_if leaves the
  // removed items in an unspecified state.
  auto animations_to_remove = std::stable_partition(
      animations_.begin(), animations_.end(),
      [animation_id](const std::unique_ptr<Animation>& animation) {
        return animation->id() != animation_id;
      });
  for (auto it = animations_to_remove; it != animations_.end(); ++it) {
    if ((*it)->target_property_id() == TargetProperty::SCROLL_OFFSET) {
      // TODO(smcgruer): Why does this check for element_animations_ existing?
      // if (element_animations_)
      //  scroll_offset_animation_was_interrupted_ = true;
    } else if (!(*it)->is_finished()) {
      animation_removed = true;
    }
  }

  animations_.erase(animations_to_remove, animations_.end());
  return animation_removed;
}

bool EffectController::AbortAnimations(TargetProperty::Type target_property,
                                       bool needs_completion,
                                       base::TimeTicks last_tick_time) {
  if (needs_completion)
    DCHECK(target_property == TargetProperty::SCROLL_OFFSET);

  bool aborted_animation = false;
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->target_property_id() == target_property &&
        !animations_[i]->is_finished()) {
      // Currently only impl-only scroll offset animations can be completed on
      // the main thread.
      if (needs_completion && animations_[i]->is_impl_only()) {
        animations_[i]->SetRunState(Animation::ABORTED_BUT_NEEDS_COMPLETION,
                                    last_tick_time);
      } else {
        animations_[i]->SetRunState(Animation::ABORTED, last_tick_time);
      }
      aborted_animation = true;
    }
  }

  return aborted_animation;
}

bool EffectController::NotifyAnimationStarted(const AnimationEvent& event) {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->group() == event.group_id &&
        animations_[i]->target_property_id() == event.target_property &&
        animations_[i]->needs_synchronized_start_time()) {
      animations_[i]->set_needs_synchronized_start_time(false);
      if (!animations_[i]->has_set_start_time())
        animations_[i]->set_start_time(event.monotonic_time);
      return true;
    }
  }
  return false;
}

bool EffectController::NotifyAnimationFinished(const AnimationEvent& event) {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->group() == event.group_id &&
        animations_[i]->target_property_id() == event.target_property) {
      animations_[i]->set_received_finished_event(true);
      return true;
    }
  }
  return false;
}

bool EffectController::NotifyAnimationAborted(const AnimationEvent& event) {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->group() == event.group_id &&
        animations_[i]->target_property_id() == event.target_property) {
      animations_[i]->SetRunState(Animation::ABORTED, event.monotonic_time);
      animations_[i]->set_received_finished_event(true);
      return true;
    }
  }
  return false;
}

bool EffectController::HasTickingAnimation() const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (!animations_[i]->is_finished())
      return true;
  }
  return false;
}

bool EffectController::HasNonDeletedAnimation() const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->run_state() != Animation::WAITING_FOR_DELETION)
      return true;
  }
  return false;
}

void EffectController::StartAnimations(base::TimeTicks monotonic_time) {
  // First collect running properties affecting each type of element.
  TargetProperties blocked_properties_for_active_elements;
  TargetProperties blocked_properties_for_pending_elements;
  std::vector<size_t> animations_waiting_for_target;

  animations_waiting_for_target.reserve(animations_.size());
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->run_state() == Animation::STARTING ||
        animations_[i]->run_state() == Animation::RUNNING) {
      int property = animations_[i]->target_property_id();
      if (animations_[i]->affects_active_elements()) {
        blocked_properties_for_active_elements[property] = true;
      }
      if (animations_[i]->affects_pending_elements()) {
        blocked_properties_for_pending_elements[property] = true;
      }
    } else if (animations_[i]->run_state() ==
               Animation::WAITING_FOR_TARGET_AVAILABILITY) {
      animations_waiting_for_target.push_back(i);
    }
  }

  for (size_t i = 0; i < animations_waiting_for_target.size(); ++i) {
    // Collect all properties for animations with the same group id (they
    // should all also be in the list of animations).
    size_t animation_index = animations_waiting_for_target[i];
    Animation* animation_waiting_for_target =
        animations_[animation_index].get();
    // Check for the run state again even though the animation was waiting
    // for target because it might have changed the run state while handling
    // previous animation in this loop (if they belong to same group).
    if (animation_waiting_for_target->run_state() ==
        Animation::WAITING_FOR_TARGET_AVAILABILITY) {
      TargetProperties enqueued_properties;
      bool affects_active_elements =
          animation_waiting_for_target->affects_active_elements();
      bool affects_pending_elements =
          animation_waiting_for_target->affects_pending_elements();
      enqueued_properties[animation_waiting_for_target->target_property_id()] =
          true;
      for (size_t j = animation_index + 1; j < animations_.size(); ++j) {
        if (animation_waiting_for_target->group() == animations_[j]->group()) {
          enqueued_properties[animations_[j]->target_property_id()] = true;
          affects_active_elements |= animations_[j]->affects_active_elements();
          affects_pending_elements |=
              animations_[j]->affects_pending_elements();
        }
      }

      // Check to see if intersection of the list of properties affected by
      // the group and the list of currently blocked properties is null, taking
      // into account the type(s) of elements affected by the group. In any
      // case, the group's target properties need to be added to the lists of
      // blocked properties.
      bool null_intersection = true;
      for (int property = TargetProperty::FIRST_TARGET_PROPERTY;
           property <= TargetProperty::LAST_TARGET_PROPERTY; ++property) {
        if (enqueued_properties[property]) {
          if (affects_active_elements) {
            if (blocked_properties_for_active_elements[property])
              null_intersection = false;
            else
              blocked_properties_for_active_elements[property] = true;
          }
          if (affects_pending_elements) {
            if (blocked_properties_for_pending_elements[property])
              null_intersection = false;
            else
              blocked_properties_for_pending_elements[property] = true;
          }
        }
      }

      // If the intersection is null, then we are free to start the animations
      // in the group.
      if (null_intersection) {
        animation_waiting_for_target->SetRunState(Animation::STARTING,
                                                  monotonic_time);
        for (size_t j = animation_index + 1; j < animations_.size(); ++j) {
          if (animation_waiting_for_target->group() ==
              animations_[j]->group()) {
            animations_[j]->SetRunState(Animation::STARTING, monotonic_time);
          }
        }
      } else {
        // TODO(smcgruer): Handle this.
        // needs_to_start_animations_ = true;
      }
    }
  }
}

void EffectController::PromoteStartedAnimations(ElementId element_id,
                                                base::TimeTicks monotonic_time,
                                                AnimationEvents* events) {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->run_state() == Animation::STARTING &&
        animations_[i]->affects_active_elements()) {
      animations_[i]->SetRunState(Animation::RUNNING, monotonic_time);
      if (!animations_[i]->has_set_start_time() &&
          !animations_[i]->needs_synchronized_start_time())
        animations_[i]->set_start_time(monotonic_time);
      if (events) {
        base::TimeTicks start_time;
        if (animations_[i]->has_set_start_time())
          start_time = animations_[i]->start_time();
        else
          start_time = monotonic_time;
        AnimationEvent started_event(
            AnimationEvent::STARTED, element_id, animations_[i]->group(),
            animations_[i]->target_property_id(), start_time);
        started_event.is_impl_only = animations_[i]->is_impl_only();
        if (started_event.is_impl_only) {
          // TODO(smcgruer): Handle.
          // Notify delegate directly, do not record the event.
          // if (animation_delegate_) {
          //  animation_delegate_->NotifyAnimationStarted(
          //      started_event.monotonic_time, started_event.target_property,
          //      started_event.group_id);
          //}
        } else {
          events->events_.push_back(started_event);
        }
      }
    }
  }
}

bool EffectController::MarkAnimationsForDeletion(ElementId element_id,
                                                 base::TimeTicks monotonic_time,
                                                 AnimationEvents* events) {
  bool marked_animations_for_deletions = false;
  std::vector<size_t> animations_with_same_group_id;

  animations_with_same_group_id.reserve(animations_.size());
  // Non-aborted animations are marked for deletion after a corresponding
  // AnimationEvent::FINISHED event is sent or received. This means that if
  // we don't have an events vector, we must ensure that non-aborted animations
  // have received a finished event before marking them for deletion.
  for (size_t i = 0; i < animations_.size(); i++) {
    int group_id = animations_[i]->group();
    if (animations_[i]->run_state() == Animation::ABORTED) {
      if (events && !animations_[i]->is_impl_only()) {
        AnimationEvent aborted_event(
            AnimationEvent::ABORTED, element_id, group_id,
            animations_[i]->target_property_id(), monotonic_time);
        events->events_.push_back(aborted_event);
      }
      // If on the compositor or on the main thread and received finish event,
      // animation can be marked for deletion.
      if (events || animations_[i]->received_finished_event()) {
        animations_[i]->SetRunState(Animation::WAITING_FOR_DELETION,
                                    monotonic_time);
        marked_animations_for_deletions = true;
      }
      continue;
    }

    // If running on the compositor and need to complete an aborted animation
    // on the main thread.
    if (events && animations_[i]->run_state() ==
                      Animation::ABORTED_BUT_NEEDS_COMPLETION) {
      AnimationEvent aborted_event(
          AnimationEvent::TAKEOVER, element_id, group_id,
          animations_[i]->target_property_id(), monotonic_time);
      aborted_event.animation_start_time = animations_[i]->start_time();
      const ScrollOffsetAnimationCurve* scroll_offset_animation_curve =
          animations_[i]->curve()->ToScrollOffsetAnimationCurve();
      aborted_event.curve = scroll_offset_animation_curve->Clone();
      // Notify the compositor that the animation is finished.
      // TODO(smcgruer): Handle this.
      // if (animation_delegate_) {
      //  animation_delegate_->NotifyAnimationFinished(
      //      aborted_event.monotonic_time, aborted_event.target_property,
      //      aborted_event.group_id);
      //}
      // Notify main thread.
      events->events_.push_back(aborted_event);

      // Remove the animation from the compositor.
      animations_[i]->SetRunState(Animation::WAITING_FOR_DELETION,
                                  monotonic_time);
      marked_animations_for_deletions = true;
      continue;
    }

    bool all_anims_with_same_id_are_finished = false;

    // Since deleting an animation on the main thread leads to its deletion
    // on the impl thread, we only mark a FINISHED main thread animation for
    // deletion once it has received a FINISHED event from the impl thread.
    bool animation_i_will_send_or_has_received_finish_event =
        animations_[i]->is_controlling_instance() ||
        animations_[i]->is_impl_only() ||
        animations_[i]->received_finished_event();
    // If an animation is finished, and not already marked for deletion,
    // find out if all other animations in the same group are also finished.
    if (animations_[i]->run_state() == Animation::FINISHED &&
        animation_i_will_send_or_has_received_finish_event) {
      // Clear the animations_with_same_group_id if it was added for
      // the previous animation's iteration.
      if (animations_with_same_group_id.size() > 0)
        animations_with_same_group_id.clear();
      all_anims_with_same_id_are_finished = true;
      for (size_t j = 0; j < animations_.size(); ++j) {
        bool animation_j_will_send_or_has_received_finish_event =
            animations_[j]->is_controlling_instance() ||
            animations_[j]->is_impl_only() ||
            animations_[j]->received_finished_event();
        if (group_id == animations_[j]->group()) {
          if (!animations_[j]->is_finished() ||
              (animations_[j]->run_state() == Animation::FINISHED &&
               !animation_j_will_send_or_has_received_finish_event)) {
            all_anims_with_same_id_are_finished = false;
            break;
          } else if (j >= i &&
                     animations_[j]->run_state() != Animation::ABORTED) {
            // Mark down the animations which belong to the same group
            // and is not yet aborted. If this current iteration finds that all
            // animations with same ID are finished, then the marked
            // animations below will be set to WAITING_FOR_DELETION in next
            // iteration.
            animations_with_same_group_id.push_back(j);
          }
        }
      }
    }

    if (all_anims_with_same_id_are_finished) {
      // We now need to remove all animations with the same group id as
      // group_id (and send along animation finished notifications, if
      // necessary).
      for (size_t j = 0; j < animations_with_same_group_id.size(); j++) {
        size_t animation_index = animations_with_same_group_id[j];
        if (events) {
          AnimationEvent finished_event(
              AnimationEvent::FINISHED, element_id,
              animations_[animation_index]->group(),
              animations_[animation_index]->target_property_id(),
              monotonic_time);
          finished_event.is_impl_only =
              animations_[animation_index]->is_impl_only();
          if (finished_event.is_impl_only) {
            // Notify delegate directly, do not record the event.
            // TODO(smcgruer): Handle this.
            // if (animation_delegate_) {
            //  animation_delegate_->NotifyAnimationFinished(
            //      finished_event.monotonic_time,
            //      finished_event.target_property, finished_event.group_id);
            //}
          } else {
            events->events_.push_back(finished_event);
          }
        }
        animations_[animation_index]->SetRunState(
            Animation::WAITING_FOR_DELETION, monotonic_time);
      }
      marked_animations_for_deletions = true;
    }
  }
  return marked_animations_for_deletions;
}

bool EffectController::MarkFinishedAnimations(base::TimeTicks monotonic_time) {
  bool animation_finished = false;

  for (size_t i = 0; i < animations_.size(); ++i) {
    if (!animations_[i]->is_finished() &&
        animations_[i]->IsFinishedAt(monotonic_time)) {
      animations_[i]->SetRunState(Animation::FINISHED, monotonic_time);
      animation_finished = true;
      // TODO(smcgruer): handle this - extra return flag?
      // SetNeedsPushProperties();
    }
    if (!animations_[i]->affects_active_elements() &&
        !animations_[i]->affects_pending_elements()) {
      switch (animations_[i]->run_state()) {
        case Animation::WAITING_FOR_TARGET_AVAILABILITY:
        case Animation::STARTING:
        case Animation::RUNNING:
        case Animation::PAUSED:
          animations_[i]->SetRunState(Animation::FINISHED, monotonic_time);
          animation_finished = true;
          break;
        default:
          break;
      }
    }
  }
  return animation_finished;
}

bool EffectController::ActivateAnimations() {
  bool animation_activated = false;

  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->affects_active_elements() !=
        animations_[i]->affects_pending_elements()) {
      animation_activated = true;
    }
    animations_[i]->set_affects_active_elements(
        animations_[i]->affects_pending_elements());
  }
  return animation_activated;
}

bool EffectController::TransformAnimationBoundsForBox(const gfx::BoxF& box,
                                                      gfx::BoxF* bounds) const {
  // Compute bounds based on animations for which is_finished() is false.
  // Do nothing if there are no such animations; in this case, it is assumed
  // that callers will take care of computing bounds based on the owning layer's
  // actual transform.
  *bounds = gfx::BoxF();
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->is_finished() ||
        animations_[i]->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    const TransformAnimationCurve* transform_animation_curve =
        animations_[i]->curve()->ToTransformAnimationCurve();
    gfx::BoxF animation_bounds;
    bool success =
        transform_animation_curve->AnimatedBoundsForBox(box, &animation_bounds);
    if (!success)
      return false;
    bounds->Union(animation_bounds);
  }
  return true;
}

bool EffectController::HasOnlyTranslationTransforms(
    ElementListType list_type) const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->is_finished() ||
        animations_[i]->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    if ((list_type == ElementListType::ACTIVE &&
         !animations_[i]->affects_active_elements()) ||
        (list_type == ElementListType::PENDING &&
         !animations_[i]->affects_pending_elements()))
      continue;

    const TransformAnimationCurve* transform_animation_curve =
        animations_[i]->curve()->ToTransformAnimationCurve();
    if (!transform_animation_curve->IsTranslation())
      return false;
  }
  return true;
}

bool EffectController::AnimationsPreserveAxisAlignment() const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->is_finished() ||
        animations_[i]->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    const TransformAnimationCurve* transform_animation_curve =
        animations_[i]->curve()->ToTransformAnimationCurve();
    if (!transform_animation_curve->PreservesAxisAlignment())
      return false;
  }
  return true;
}

bool EffectController::AnimationStartScale(ElementListType list_type,
                                           float* start_scale) const {
  *start_scale = 0.f;
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->is_finished() ||
        animations_[i]->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    if ((list_type == ElementListType::ACTIVE &&
         !animations_[i]->affects_active_elements()) ||
        (list_type == ElementListType::PENDING &&
         !animations_[i]->affects_pending_elements()))
      continue;

    bool forward_direction = true;
    switch (animations_[i]->direction()) {
      case Animation::Direction::NORMAL:
      case Animation::Direction::ALTERNATE_NORMAL:
        forward_direction = animations_[i]->playback_rate() >= 0.0;
        break;
      case Animation::Direction::REVERSE:
      case Animation::Direction::ALTERNATE_REVERSE:
        forward_direction = animations_[i]->playback_rate() < 0.0;
        break;
    }

    const TransformAnimationCurve* transform_animation_curve =
        animations_[i]->curve()->ToTransformAnimationCurve();
    float animation_start_scale = 0.f;
    if (!transform_animation_curve->AnimationStartScale(forward_direction,
                                                        &animation_start_scale))
      return false;
    *start_scale = std::max(*start_scale, animation_start_scale);
  }
  return true;
}

bool EffectController::MaximumTargetScale(ElementListType list_type,
                                          float* max_scale) const {
  *max_scale = 0.f;
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (animations_[i]->is_finished() ||
        animations_[i]->target_property_id() != TargetProperty::TRANSFORM)
      continue;

    if ((list_type == ElementListType::ACTIVE &&
         !animations_[i]->affects_active_elements()) ||
        (list_type == ElementListType::PENDING &&
         !animations_[i]->affects_pending_elements()))
      continue;

    bool forward_direction = true;
    switch (animations_[i]->direction()) {
      case Animation::Direction::NORMAL:
      case Animation::Direction::ALTERNATE_NORMAL:
        forward_direction = animations_[i]->playback_rate() >= 0.0;
        break;
      case Animation::Direction::REVERSE:
      case Animation::Direction::ALTERNATE_REVERSE:
        forward_direction = animations_[i]->playback_rate() < 0.0;
        break;
    }

    const TransformAnimationCurve* transform_animation_curve =
        animations_[i]->curve()->ToTransformAnimationCurve();
    float animation_scale = 0.f;
    if (!transform_animation_curve->MaximumTargetScale(forward_direction,
                                                       &animation_scale))
      return false;
    *max_scale = std::max(*max_scale, animation_scale);
  }

  return true;
}

bool EffectController::IsPotentiallyAnimatingProperty(
    TargetProperty::Type target_property,
    ElementListType list_type) const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (!animations_[i]->is_finished() &&
        animations_[i]->target_property_id() == target_property) {
      if ((list_type == ElementListType::ACTIVE &&
           animations_[i]->affects_active_elements()) ||
          (list_type == ElementListType::PENDING &&
           animations_[i]->affects_pending_elements()))
        return true;
    }
  }
  return false;
}

bool EffectController::IsCurrentlyAnimatingProperty(
    TargetProperty::Type target_property,
    ElementListType list_type,
    base::TimeTicks last_tick_time) const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    if (!animations_[i]->is_finished() &&
        animations_[i]->InEffect(last_tick_time) &&
        animations_[i]->target_property_id() == target_property) {
      if ((list_type == ElementListType::ACTIVE &&
           animations_[i]->affects_active_elements()) ||
          (list_type == ElementListType::PENDING &&
           animations_[i]->affects_pending_elements()))
        return true;
    }
  }
  return false;
}

Animation* EffectController::GetAnimation(
    TargetProperty::Type target_property) const {
  for (size_t i = 0; i < animations_.size(); ++i) {
    size_t index = animations_.size() - i - 1;
    if (animations_[index]->target_property_id() == target_property)
      return animations_[index].get();
  }
  return nullptr;
}

Animation* EffectController::GetAnimationById(int animation_id) const {
  for (size_t i = 0; i < animations_.size(); ++i)
    if (animations_[i]->id() == animation_id)
      return animations_[i].get();
  return nullptr;
}

void EffectController::GetPropertyAnimationState(
    PropertyAnimationState* pending_state,
    PropertyAnimationState* active_state,
    base::TimeTicks last_tick_time) const {
  pending_state->Clear();
  active_state->Clear();

  for (const auto& animation : animations_) {
    if (!animation->is_finished()) {
      bool in_effect = animation->InEffect(last_tick_time);
      bool active = animation->affects_active_elements();
      bool pending = animation->affects_pending_elements();
      int property = animation->target_property_id();

      if (pending)
        pending_state->potentially_animating[property] = true;
      if (pending && in_effect)
        pending_state->currently_running[property] = true;

      if (active)
        active_state->potentially_animating[property] = true;
      if (active && in_effect)
        active_state->currently_running[property] = true;
    }
  }
}

void EffectController::PurgeAnimationsMarkedForDeletion(bool impl_only) {
  base::EraseIf(
      animations_, [impl_only](const std::unique_ptr<Animation>& animation) {
        return animation->run_state() == Animation::WAITING_FOR_DELETION &&
               (!impl_only || animation->is_impl_only());
      });
}

std::string EffectController::AnimationsToString() const {
  std::string str;
  for (size_t i = 0; i < animations_.size(); i++) {
    if (i > 0)
      str.append(", ");
    str.append(animations_[i]->ToString());
  }
  return str;
}

}  // namespace cc
