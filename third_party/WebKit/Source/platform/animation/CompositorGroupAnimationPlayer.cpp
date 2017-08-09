// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/animation/CompositorGroupAnimationPlayer.h"

#include "cc/animation/animation_id_provider.h"
#include "cc/animation/animation_timeline.h"
#include "platform/animation/CompositorAnimation.h"
#include "platform/animation/CompositorAnimationDelegate.h"

namespace blink {

CompositorGroupAnimationPlayer::CompositorGroupAnimationPlayer()
    // Should use NextGroupId?
    : group_animation_player_(cc::GroupAnimationPlayer::Create(
          cc::AnimationIdProvider::NextGroupPlayerId())),
      delegate_() {}

CompositorGroupAnimationPlayer::~CompositorGroupAnimationPlayer() {
  SetAnimationDelegate(nullptr);
  // Detach player from timeline, otherwise it stays there (leaks) until
  // compositor shutdown.
  if (group_animation_player_->animation_timeline()) {
    group_animation_player_->animation_timeline()->DetachGroupPlayer(
        group_animation_player_);
  }
}

cc::GroupAnimationPlayer*
CompositorGroupAnimationPlayer::CcGroupAnimationPlayer() const {
  return group_animation_player_.get();
}

void CompositorGroupAnimationPlayer::SetAnimationDelegate(
    CompositorAnimationDelegate* delegate) {
  delegate_ = delegate;
  // TODO (yigu): Move delegate to GroupAnimationPlayer.
  //  animation_player_->set_animation_delegate(delegate ? this : nullptr);
}

// void CompositorGroupAnimationPlayer::AddAnimation(
//    std::unique_ptr<CompositorAnimation> animation) {
//  animation_player_->AddAnimation(animation->ReleaseCcAnimation());
//}
//
// void CompositorGroupAnimationPlayer::RemoveAnimation(int animation_id) {
//  animation_player_->RemoveAnimation(animation_id);
//}
//
// void CompositorGroupAnimationPlayer::PauseAnimation(int animation_id,
//                                               double time_offset) {
//  animation_player_->PauseAnimation(animation_id, time_offset);
//}
//
// void CompositorGroupAnimationPlayer::AbortAnimation(int animation_id) {
//  animation_player_->AbortAnimation(animation_id);
//}
//
void CompositorGroupAnimationPlayer::NotifyAnimationStarted(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  if (delegate_) {
    delegate_->NotifyAnimationStarted(
        (monotonic_time - base::TimeTicks()).InSecondsF(), group);
  }
}

void CompositorGroupAnimationPlayer::NotifyAnimationFinished(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  if (delegate_) {
    delegate_->NotifyAnimationFinished(
        (monotonic_time - base::TimeTicks()).InSecondsF(), group);
  }
}

void CompositorGroupAnimationPlayer::NotifyAnimationAborted(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  if (delegate_) {
    delegate_->NotifyAnimationAborted(
        (monotonic_time - base::TimeTicks()).InSecondsF(), group);
  }
}

void CompositorGroupAnimationPlayer::NotifyAnimationTakeover(
    base::TimeTicks monotonic_time,
    int target_property,
    base::TimeTicks animation_start_time,
    std::unique_ptr<cc::AnimationCurve> curve) {
  if (delegate_) {
    delegate_->NotifyAnimationTakeover(
        (monotonic_time - base::TimeTicks()).InSecondsF(),
        (animation_start_time - base::TimeTicks()).InSecondsF(),
        std::move(curve));
  }
}

}  // namespace blink
