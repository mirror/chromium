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
  AddTicker(base::MakeUnique<AnimationTicker>(NextTickerId()));
}

SingleAnimationPlayer::SingleAnimationPlayer(int id, size_t ticker_id)
    : AnimationPlayer(id) {
  DCHECK(id_);
  AddTicker(base::MakeUnique<AnimationTicker>(ticker_id));
}

SingleAnimationPlayer::~SingleAnimationPlayer() {}

AnimationTicker* SingleAnimationPlayer::GetTicker() const {
  DCHECK_EQ(tickers_.size(), 1u);
  return tickers_[0].get();
}

scoped_refptr<AnimationPlayer> SingleAnimationPlayer::CreateImplInstance()
    const {
  DCHECK(GetTicker());
  scoped_refptr<SingleAnimationPlayer> player =
      base::WrapRefCounted(new SingleAnimationPlayer(id(), GetTicker()->id()));
  return player;
}

ElementId SingleAnimationPlayer::element_id() const {
  return element_id_of_ticker(GetTicker()->id());
}

void SingleAnimationPlayer::AttachElement(ElementId element_id) {
  AttachElementForTicker(element_id, GetTicker()->id());
}

AnimationTicker* SingleAnimationPlayer::animation_ticker() const {
  return GetTicker();
}

void SingleAnimationPlayer::AddAnimation(std::unique_ptr<Animation> animation) {
  AddAnimationForTicker(std::move(animation), GetTicker()->id());
}

void SingleAnimationPlayer::PauseAnimation(int animation_id,
                                           double time_offset) {
  PauseAnimationForTicker(animation_id, time_offset, GetTicker()->id());
}

void SingleAnimationPlayer::RemoveAnimation(int animation_id) {
  RemoveAnimationForTicker(animation_id, GetTicker()->id());
}

void SingleAnimationPlayer::AbortAnimation(int animation_id) {
  AbortAnimationForTicker(animation_id, GetTicker()->id());
}

bool SingleAnimationPlayer::NotifyAnimationFinishedForTesting(
    TargetProperty::Type target_property,
    int group_id) {
  AnimationEvent event(AnimationEvent::FINISHED, GetTicker()->element_id(),
                       group_id, target_property, base::TimeTicks());
  return GetTicker()->NotifyAnimationFinished(event);
}

Animation* SingleAnimationPlayer::GetAnimation(
    TargetProperty::Type target_property) const {
  return GetAnimationForTicker(target_property, GetTicker()->id());
}

}  // namespace cc
