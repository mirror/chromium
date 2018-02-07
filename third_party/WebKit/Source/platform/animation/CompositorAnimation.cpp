// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/animation/CompositorAnimation.h"

#include "base/memory/ptr_util.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_id_provider.h"
#include "cc/animation/keyframed_animation_curve.h"
#include "platform/animation/CompositorAnimationCurve.h"
#include "platform/animation/CompositorFilterAnimationCurve.h"
#include "platform/animation/CompositorFloatAnimationCurve.h"
#include "platform/animation/CompositorScrollOffsetAnimationCurve.h"
#include "platform/animation/CompositorTransformAnimationCurve.h"
#include <memory>

using cc::KeyframeModel;
using cc::AnimationIdProvider;

using blink::CompositorAnimation;
using blink::CompositorAnimationCurve;

namespace blink {

CompositorAnimation::CompositorAnimation(
    const CompositorAnimationCurve& curve,
    CompositorTargetProperty::Type target_property,
    int keyframe_model_id,
    int group_id) {
  if (!keyframe_model_id)
    keyframe_model_id = AnimationIdProvider::NextKeyframeModelId();
  if (!group_id)
    group_id = AnimationIdProvider::NextGroupId();

  keyframe_model_ =
      KeyframeModel::Create(curve.CloneToAnimationCurve(), keyframe_model_id,
                            group_id, target_property);
}

CompositorAnimation::~CompositorAnimation() = default;

int CompositorAnimation::Id() const {
  return keyframe_model_->id();
}

int CompositorAnimation::Group() const {
  return keyframe_model_->group();
}

CompositorTargetProperty::Type CompositorAnimation::TargetProperty() const {
  return static_cast<CompositorTargetProperty::Type>(
      keyframe_model_->target_property_id());
}

double CompositorAnimation::Iterations() const {
  return keyframe_model_->iterations();
}

void CompositorAnimation::SetIterations(double n) {
  keyframe_model_->set_iterations(n);
}

double CompositorAnimation::IterationStart() const {
  return keyframe_model_->iteration_start();
}

void CompositorAnimation::SetIterationStart(double iteration_start) {
  keyframe_model_->set_iteration_start(iteration_start);
}

double CompositorAnimation::StartTime() const {
  return (keyframe_model_->start_time() - base::TimeTicks()).InSecondsF();
}

void CompositorAnimation::SetStartTime(double monotonic_time) {
  keyframe_model_->set_start_time(base::TimeTicks::FromInternalValue(
      monotonic_time * base::Time::kMicrosecondsPerSecond));
}

double CompositorAnimation::TimeOffset() const {
  return keyframe_model_->time_offset().InSecondsF();
}

void CompositorAnimation::SetTimeOffset(double monotonic_time) {
  keyframe_model_->set_time_offset(
      base::TimeDelta::FromSecondsD(monotonic_time));
}

blink::CompositorAnimation::Direction CompositorAnimation::GetDirection()
    const {
  return keyframe_model_->direction();
}

void CompositorAnimation::SetDirection(Direction direction) {
  keyframe_model_->set_direction(direction);
}

double CompositorAnimation::PlaybackRate() const {
  return keyframe_model_->playback_rate();
}

void CompositorAnimation::SetPlaybackRate(double playback_rate) {
  keyframe_model_->set_playback_rate(playback_rate);
}

blink::CompositorAnimation::FillMode CompositorAnimation::GetFillMode() const {
  return keyframe_model_->fill_mode();
}

void CompositorAnimation::SetFillMode(FillMode fill_mode) {
  keyframe_model_->set_fill_mode(fill_mode);
}

std::unique_ptr<cc::KeyframeModel> CompositorAnimation::ReleaseCcAnimation() {
  keyframe_model_->set_needs_synchronized_start_time(true);
  return std::move(keyframe_model_);
}

std::unique_ptr<CompositorFloatAnimationCurve>
CompositorAnimation::FloatCurveForTesting() const {
  const cc::AnimationCurve* curve = keyframe_model_->curve();
  DCHECK_EQ(cc::AnimationCurve::FLOAT, curve->Type());

  auto keyframed_curve = base::WrapUnique(
      static_cast<cc::KeyframedFloatAnimationCurve*>(curve->Clone().release()));
  return CompositorFloatAnimationCurve::CreateForTesting(
      std::move(keyframed_curve));
}

}  // namespace blink
