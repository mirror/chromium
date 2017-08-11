// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/animation_player.h"

#include <algorithm>

#include "base/stl_util.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_player.h"
#include "cc/animation/animation_target.h"
#include "cc/animation/keyframed_animation_curve.h"
#include "cc/base/math_util.h"
#include "chrome/browser/vr/elements/ui_element.h"

using cc::MathUtil;

namespace vr {

namespace {

static constexpr float kTolerance = 1e-5;

static int s_next_animation_id = 1;
static int s_next_group_id = 1;

void ReverseAnimation(base::TimeTicks monotonic_time,
                      cc::Animation* animation) {
  animation->set_direction(animation->direction() ==
                                   cc::Animation::Direction::NORMAL
                               ? cc::Animation::Direction::REVERSE
                               : cc::Animation::Direction::NORMAL);
  // Our goal here is to reverse the given animation. That is, if
  // we're 20% of the way through the animation in the forward direction, we'd
  // like to be 80% of the way of the reversed animation (so it will end
  // quickly).
  //
  // We can modify our "progress" through an animation by modifying the "time
  // offset", a value added to the current time by the animation system before
  // applying any other adjustments.
  //
  // Let our start time be s, our current time be t, and our final time (or
  // duration) be d. After reversing the animation, we would like to start
  // sampling from d - t as depicted below.
  //
  //  Forward:
  //  s    t                         d
  //  |----|-------------------------|
  //
  //  Reversed:
  //  s                         t    d
  //  |----|--------------------|----|
  //       -----time-offset----->
  //
  // Now, if we let o represent our desired offset, we need to ensure that
  //   t = d - (o + t)
  //
  // That is, sampling at the current time in either the forward or reverse
  // curves must result in the same value, otherwise we'll get jank.
  //
  // This implies that,
  //   0 = d - o - 2t
  //   o = d - 2t
  //
  // Now if there was a previous offset, we must adjust d by that offset before
  // performing this computation, so it becomes d - o_old - 2t:
  animation->set_time_offset(animation->curve()->Duration() -
                             animation->time_offset() -
                             (2 * (monotonic_time - animation->start_time())));
}

std::unique_ptr<cc::CubicBezierTimingFunction>
CreateTransitionTimingFunction() {
  return cc::CubicBezierTimingFunction::CreatePreset(
      cc::CubicBezierTimingFunction::EaseType::EASE);
}

base::TimeDelta GetStartTime(cc::Animation* animation) {
  if (animation->direction() == cc::Animation::Direction::NORMAL) {
    return base::TimeDelta();
  }
  return animation->curve()->Duration();
}

base::TimeDelta GetEndTime(cc::Animation* animation) {
  if (animation->direction() == cc::Animation::Direction::REVERSE) {
    return base::TimeDelta();
  }
  return animation->curve()->Duration();
}

template <typename T>
struct assert_false : std::false_type {};

#define PROXY_TYPE_BASE(proxy_type, proxied_type_base)             \
  template <typename ValueType>                                    \
  struct proxy_type {                                              \
    static_assert(assert_false<ValueType>::value,                  \
                  "Cannot make proxy type for " #proxied_type_base \
                  " subclass");                                    \
  };

#define PROXY_TYPE(proxy_type, value_type, proxied_type) \
  template <>                                            \
  struct proxy_type<value_type> {                        \
    typedef proxied_type Type;                           \
  };

PROXY_TYPE_BASE(AnimationCurve, cc::AnimationCurve)
PROXY_TYPE(AnimationCurve, float, cc::FloatAnimationCurve)
PROXY_TYPE(AnimationCurve, cc::TransformOperations, cc::TransformAnimationCurve)
PROXY_TYPE(AnimationCurve, gfx::SizeF, cc::SizeAnimationCurve)
PROXY_TYPE(AnimationCurve, SkColor, cc::ColorAnimationCurve)
PROXY_TYPE(AnimationCurve, bool, cc::BooleanAnimationCurve)

PROXY_TYPE_BASE(KeyframedAnimationCurve, cc::KeyframedAnimationCurve)
PROXY_TYPE(KeyframedAnimationCurve, float, cc::KeyframedFloatAnimationCurve)
PROXY_TYPE(KeyframedAnimationCurve,
           cc::TransformOperations,
           cc::KeyframedTransformAnimationCurve)
PROXY_TYPE(KeyframedAnimationCurve, gfx::SizeF, cc::KeyframedSizeAnimationCurve)
PROXY_TYPE(KeyframedAnimationCurve, SkColor, cc::KeyframedColorAnimationCurve)
PROXY_TYPE(KeyframedAnimationCurve, bool, cc::KeyframedBooleanAnimationCurve)

PROXY_TYPE_BASE(Keyframe, cc::Keyframe)
PROXY_TYPE(Keyframe, float, cc::FloatKeyframe)
PROXY_TYPE(Keyframe, cc::TransformOperations, cc::TransformKeyframe)
PROXY_TYPE(Keyframe, gfx::SizeF, cc::SizeKeyframe)
PROXY_TYPE(Keyframe, SkColor, cc::ColorKeyframe)
PROXY_TYPE(Keyframe, bool, cc::BooleanKeyframe)

template <typename ValueType>
const typename AnimationCurve<ValueType>::Type* ToAnimationCurve(
    const cc::AnimationCurve& curve) {
  static_assert(
      assert_false<ValueType>::value,
      "Cannot make specialized cc::AnimationCurve for this value type");
}

#define TO_ANIMATION_CURVE(value_type, to_function)               \
  template <>                                                     \
  const typename AnimationCurve<value_type>::Type*                \
  ToAnimationCurve<value_type>(const cc::AnimationCurve& curve) { \
    return curve.to_function();                                   \
  }

TO_ANIMATION_CURVE(float, ToFloatAnimationCurve);
TO_ANIMATION_CURVE(cc::TransformOperations, ToTransformAnimationCurve);
TO_ANIMATION_CURVE(gfx::SizeF, ToSizeAnimationCurve);
TO_ANIMATION_CURVE(SkColor, ToColorAnimationCurve);
TO_ANIMATION_CURVE(bool, ToBooleanAnimationCurve);

template <typename ValueType>
void NotifyClientValueAnimated(cc::AnimationTarget* target,
                               const ValueType& value,
                               int target_property,
                               cc::Animation* animation) {
  static_assert(assert_false<ValueType>::value,
                "Cannot notify client for this value type");
}

#define NOTIFY_CLIENT_VALUE_ANIMATED(value_type, notify_function) \
  template <>                                                     \
  void NotifyClientValueAnimated<value_type>(                     \
      cc::AnimationTarget * target, const value_type& value,      \
      int target_property, cc::Animation* animation) {            \
    target->notify_function(value, target_property, animation);   \
  }

NOTIFY_CLIENT_VALUE_ANIMATED(float, NotifyClientFloatAnimated);
NOTIFY_CLIENT_VALUE_ANIMATED(cc::TransformOperations,
                             NotifyClientTransformOperationsAnimated);
NOTIFY_CLIENT_VALUE_ANIMATED(gfx::SizeF, NotifyClientSizeAnimated);
NOTIFY_CLIENT_VALUE_ANIMATED(SkColor, NotifyClientColorAnimated);
NOTIFY_CLIENT_VALUE_ANIMATED(bool, NotifyClientBooleanAnimated);

bool SufficientlyEqual(float lhs, float rhs) {
  return MathUtil::ApproximatelyEqual(lhs, rhs, kTolerance);
}

bool SufficientlyEqual(const cc::TransformOperations& lhs,
                       const cc::TransformOperations& rhs) {
  return lhs.ApproximatelyEqual(rhs, kTolerance);
}

bool SufficientlyEqual(const gfx::SizeF& lhs, const gfx::SizeF& rhs) {
  return MathUtil::ApproximatelyEqual(lhs.width(), rhs.width(), kTolerance) &&
         MathUtil::ApproximatelyEqual(lhs.height(), rhs.height(), kTolerance);
}

bool SufficientlyEqual(SkColor lhs, SkColor rhs) {
  return lhs == rhs;
}

bool SufficientlyEqual(bool lhs, bool rhs) {
  return lhs == rhs;
}

}  // namespace

AnimationPlayer::AnimationPlayer() {}
AnimationPlayer::~AnimationPlayer() {}

int AnimationPlayer::GetNextAnimationId() {
  return s_next_animation_id++;
}

int AnimationPlayer::GetNextGroupId() {
  return s_next_group_id++;
}

void AnimationPlayer::AddAnimation(std::unique_ptr<cc::Animation> animation) {
  animations_.push_back(std::move(animation));
}

void AnimationPlayer::RemoveAnimation(int animation_id) {
  base::EraseIf(
      animations_,
      [animation_id](const std::unique_ptr<cc::Animation>& animation) {
        return animation->id() == animation_id;
      });
}

void AnimationPlayer::RemoveAnimations(int target_property) {
  base::EraseIf(
      animations_,
      [target_property](const std::unique_ptr<cc::Animation>& animation) {
        return animation->target_property_id() == target_property;
      });
}

void AnimationPlayer::StartAnimations(base::TimeTicks monotonic_time) {
  // TODO(vollick): support groups. crbug.com/742358
  cc::TargetProperties animated_properties;
  for (auto& animation : animations_) {
    if (animation->run_state() == cc::Animation::RUNNING ||
        animation->run_state() == cc::Animation::PAUSED) {
      animated_properties[animation->target_property_id()] = true;
    }
  }
  for (auto& animation : animations_) {
    if (!animated_properties[animation->target_property_id()] &&
        animation->run_state() ==
            cc::Animation::WAITING_FOR_TARGET_AVAILABILITY) {
      animated_properties[animation->target_property_id()] = true;
      animation->SetRunState(cc::Animation::RUNNING, monotonic_time);
      animation->set_start_time(monotonic_time);
    }
  }
}

void AnimationPlayer::Tick(base::TimeTicks monotonic_time) {
  DCHECK(target_);

  StartAnimations(monotonic_time);

  for (auto& animation : animations_) {
    cc::AnimationPlayer::TickAnimation(monotonic_time, animation.get(),
                                       target_);
  }

  // Remove finished animations.
  base::EraseIf(
      animations_,
      [monotonic_time](const std::unique_ptr<cc::Animation>& animation) {
        return !animation->is_finished() &&
               animation->IsFinishedAt(monotonic_time);
      });

  StartAnimations(monotonic_time);
}

void AnimationPlayer::SetTransitionedProperties(
    const std::set<int>& properties) {
  transition_.target_properties = properties;
}

void AnimationPlayer::TransitionFloatTo(base::TimeTicks monotonic_time,
                                        int target_property,
                                        float current,
                                        float target) {
  TransitionValueTo<float>(monotonic_time, target_property, current, target);
}

void AnimationPlayer::TransitionTransformOperationsTo(
    base::TimeTicks monotonic_time,
    int target_property,
    const cc::TransformOperations& current,
    const cc::TransformOperations& target) {
  TransitionValueTo<cc::TransformOperations>(monotonic_time, target_property,
                                             current, target);
}

void AnimationPlayer::TransitionSizeTo(base::TimeTicks monotonic_time,
                                       int target_property,
                                       const gfx::SizeF& current,
                                       const gfx::SizeF& target) {
  TransitionValueTo<gfx::SizeF>(monotonic_time, target_property, current,
                                target);
}

void AnimationPlayer::TransitionColorTo(base::TimeTicks monotonic_time,
                                        int target_property,
                                        SkColor current,
                                        SkColor target) {
  TransitionValueTo<SkColor>(monotonic_time, target_property, current, target);
}

void AnimationPlayer::TransitionBooleanTo(base::TimeTicks monotonic_time,
                                          int target_property,
                                          bool current,
                                          bool target) {
  TransitionValueTo<bool>(monotonic_time, target_property, current, target);
}

cc::Animation* AnimationPlayer::GetRunningAnimationForProperty(
    int target_property) const {
  for (auto& animation : animations_) {
    if ((animation->run_state() == cc::Animation::RUNNING ||
         animation->run_state() == cc::Animation::PAUSED) &&
        animation->target_property_id() == target_property) {
      return animation.get();
    }
  }
  return nullptr;
}

template <typename ValueType>
void AnimationPlayer::TransitionValueTo(base::TimeTicks monotonic_time,
                                        int target_property,
                                        const ValueType& current,
                                        const ValueType& target) {
  DCHECK(target_);

  if (transition_.target_properties.find(target_property) ==
      transition_.target_properties.end()) {
    NotifyClientValueAnimated<ValueType>(target_, target, target_property,
                                         nullptr);
    return;
  }

  cc::Animation* running_animation =
      GetRunningAnimationForProperty(target_property);

  if (running_animation) {
    const auto* curve =
        ToAnimationCurve<ValueType>(*running_animation->curve());
    if (SufficientlyEqual(target,
                          curve->GetValue(GetEndTime(running_animation)))) {
      return;
    }
    if (SufficientlyEqual(target,
                          curve->GetValue(GetStartTime(running_animation)))) {
      ReverseAnimation(monotonic_time, running_animation);
      return;
    }
  } else if (SufficientlyEqual(target, current)) {
    return;
  }

  RemoveAnimations(target_property);

  auto curve = KeyframedAnimationCurve<ValueType>::Type::Create();

  curve->AddKeyframe(Keyframe<ValueType>::Type::Create(
      base::TimeDelta(), current, CreateTransitionTimingFunction()));

  curve->AddKeyframe(Keyframe<ValueType>::Type::Create(
      transition_.duration, target, CreateTransitionTimingFunction()));

  std::unique_ptr<cc::Animation> animation(
      cc::Animation::Create(std::move(curve), GetNextAnimationId(),
                            GetNextGroupId(), target_property));

  AddAnimation(std::move(animation));
}

bool AnimationPlayer::IsAnimatingProperty(int property) const {
  for (auto& animation : animations_) {
    if (animation->target_property_id() == property)
      return true;
  }
  return false;
}

}  // namespace vr
