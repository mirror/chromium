// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/animation_creator.h"

namespace vr {

AnimationCreator::AnimationCreator() : end_time_(usToDelta(10000)) {}

std::unique_ptr<cc::Animation> AnimationCreator::transform_animation(
    int id,
    int group) {
  std::unique_ptr<cc::KeyframedTransformAnimationCurve> curve(
      cc::KeyframedTransformAnimationCurve::Create());
  curve->AddKeyframe(
      cc::TransformKeyframe::Create(begin_time_, from_operations_, nullptr));
  curve->AddKeyframe(
      cc::TransformKeyframe::Create(end_time_, to_operations_, nullptr));
  std::unique_ptr<cc::Animation> animation(cc::Animation::Create(
      std::move(curve), id, group, cc::TargetProperty::TRANSFORM));
  return animation;
}

std::unique_ptr<cc::Animation> AnimationCreator::size_animation(int id,
                                                                int group) {
  std::unique_ptr<cc::KeyframedSizeAnimationCurve> curve(
      cc::KeyframedSizeAnimationCurve::Create());
  curve->AddKeyframe(
      cc::SizeKeyframe::Create(begin_time_, from_size_, nullptr));
  curve->AddKeyframe(cc::SizeKeyframe::Create(end_time_, to_size_, nullptr));
  std::unique_ptr<cc::Animation> animation(cc::Animation::Create(
      std::move(curve), id, group, cc::TargetProperty::BOUNDS));
  return animation;
}

base::TimeTicks AnimationCreator::usToTicks(uint64_t us) {
  return base::TimeTicks::FromInternalValue(us);
}

base::TimeDelta AnimationCreator::usToDelta(uint64_t us) {
  return base::TimeDelta::FromInternalValue(us);
}

}  // namespace vr
