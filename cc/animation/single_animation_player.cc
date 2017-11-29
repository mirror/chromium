// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/single_animation_player.h"

#include <inttypes.h>
#include <algorithm>

#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "cc/animation/animation_delegate.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_id_provider.h"
#include "cc/animation/animation_ticker.h"
#include "cc/animation/animation_timeline.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/animation/transform_operations.h"
#include "cc/trees/property_animation_state.h"

namespace cc {

scoped_refptr<SingleAnimationPlayer> SingleAnimationPlayer::Create(int id) {
  return base::WrapRefCounted(new SingleAnimationPlayer(id));
}

SingleAnimationPlayer::SingleAnimationPlayer(int id) : AnimationPlayer(id) {
  DCHECK(id_);
  AddTicker(
      base::MakeUnique<AnimationTicker>(AnimationIdProvider::NextTickerId()));
}

SingleAnimationPlayer::~SingleAnimationPlayer() {}

AnimationTicker* SingleAnimationPlayer::GetUniqueTicker() const {
  DCHECK(!id_to_ticker_map_.empty());
  return id_to_ticker_map_.begin()->second.get();
}

scoped_refptr<AnimationPlayer> SingleAnimationPlayer::CreateImplInstance()
    const {
  scoped_refptr<SingleAnimationPlayer> player =
      SingleAnimationPlayer::Create(id());
  return player;
}

ElementId SingleAnimationPlayer::element_id() const {
  return element_id_of_ticker(GetUniqueTicker()->id());
}

void SingleAnimationPlayer::AttachElement(ElementId element_id) {
  AttachElementForTicker(element_id, GetUniqueTicker()->id());
}

AnimationTicker* SingleAnimationPlayer::animation_ticker() const {
  return GetUniqueTicker();
}

void SingleAnimationPlayer::AddAnimation(std::unique_ptr<Animation> animation) {
  AddAnimationForTicker(std::move(animation), GetUniqueTicker()->id());
}

void SingleAnimationPlayer::PauseAnimation(int animation_id,
                                           double time_offset) {
  PauseAnimationForTicker(animation_id, time_offset, GetUniqueTicker()->id());
}

void SingleAnimationPlayer::RemoveAnimation(int animation_id) {
  RemoveAnimationForTicker(animation_id, GetUniqueTicker()->id());
}

void SingleAnimationPlayer::AbortAnimation(int animation_id) {
  AbortAnimationForTicker(animation_id, GetUniqueTicker()->id());
}

void SingleAnimationPlayer::AbortAnimations(
    TargetProperty::Type target_property,
    bool needs_completion) {
  AbortAnimationsForTicker(target_property, needs_completion,
                           GetUniqueTicker()->id());
}

bool SingleAnimationPlayer::NotifyAnimationFinishedForTesting(
    TargetProperty::Type target_property,
    int group_id) {
  AnimationEvent event(AnimationEvent::FINISHED,
                       GetUniqueTicker()->element_id(), group_id,
                       target_property, base::TimeTicks());
  return GetUniqueTicker()->NotifyAnimationFinished(event);
}

Animation* SingleAnimationPlayer::GetAnimation(
    TargetProperty::Type target_property) const {
  return GetAnimationForTicker(target_property, GetUniqueTicker()->id());
}

}  // namespace cc
