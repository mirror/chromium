// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/worklet_animation_player.h"

#include "base/memory/ptr_util.h"
#include "base/time/time.h"

#include "base/logging.h"

namespace cc {

WorkletAnimationPlayer::WorkletAnimationPlayer(int id, const std::string& name)
    : AnimationPlayer(id), name_(name) {}

WorkletAnimationPlayer::~WorkletAnimationPlayer() {}

scoped_refptr<WorkletAnimationPlayer> WorkletAnimationPlayer::Create(
    int id,
    const std::string& name) {
  return make_scoped_refptr(new WorkletAnimationPlayer(id, name));
}

scoped_refptr<AnimationPlayer> WorkletAnimationPlayer::CreateImplInstance()
    const {
  return make_scoped_refptr(new WorkletAnimationPlayer(id(), name()));
}

void WorkletAnimationPlayer::SetLocalTime(base::TimeDelta local_time) {
  local_time_ = local_time;
  SetNeedsPushProperties();
}

void WorkletAnimationPlayer::PushPropertiesToImplThread(
    AnimationPlayer* animation_player_impl) {
  AnimationPlayer::PushPropertiesToImplThread(animation_player_impl);
  static_cast<WorkletAnimationPlayer*>(animation_player_impl)
      ->SetLocalTime(local_time_);
}

bool WorkletAnimationPlayer::IsWorkletAnimationPlayer() const {
  return true;
}

base::TimeTicks WorkletAnimationPlayer::GetTickTimeForAnimation(
    base::TimeTicks monotonic_time,
    Animation* animation) {
  // Animation player local time is equivalent to animation active time. So
  // we have to convert it from active time to monotonic time.
  return animation->ConvertFromActiveTime(local_time_);
}

}  // namespace cc
