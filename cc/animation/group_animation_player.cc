// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/group_animation_player.h"

#include <algorithm>

#include "base/stl_util.h"
#include "cc/animation/animation_delegate.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_player.h"
#include "cc/animation/animation_timeline.h"
#include "cc/animation/scroll_offset_animation_curve.h"
#include "cc/trees/property_animation_state.h"

namespace cc {

scoped_refptr<GroupAnimationPlayer> GroupAnimationPlayer::Create(int id) {
  return make_scoped_refptr(new GroupAnimationPlayer(id));
}

GroupAnimationPlayer::GroupAnimationPlayer(int id) : id_(id) {
  DCHECK(id_);
}

GroupAnimationPlayer::~GroupAnimationPlayer() {}

scoped_refptr<GroupAnimationPlayer> GroupAnimationPlayer::CreateImplInstance()
    const {
  scoped_refptr<GroupAnimationPlayer> player =
      GroupAnimationPlayer::Create(id());
  return player;
}

void GroupAnimationPlayer::RegisterPlayer(AnimationPlayer& animation_player) {
  DCHECK(animation_host_);

  // Create ElementAnimations or re-use existing.
  for (auto& animation_player : animation_players_) {
    animation_host_->RegisterPlayerForElement(animation_player.element_id(),
                                              &animation_player);
    // Get local reference to shared ElementAnimations.
    BindElementAnimations(animation_player);
  }
}

void GroupAnimationPlayer::UnregisterPlayer(AnimationPlayer& animation_player) {
  DCHECK(animation_host_);

  // Destroy ElementAnimations or release it if it's still needed.
  for (auto& animation_player : animation_players_) {
    UnbindElementAnimations(animation_player);
    animation_host_->UnregisterPlayerForElement(animation_player.element_id(),
                                                &animation_player);
  }
}

void GroupAnimationPlayer::BindElementAnimations(
    AnimationPlayer& animation_player) {
  DCHECK(animation_player.element_animations());

  if (animation_player.has_any_animation())
    animation_player.AnimationAdded();

  SetNeedsPushProperties(animation_player);
}

void GroupAnimationPlayer::UnbindElementAnimations(
    AnimationPlayer& animation_player) {
  SetNeedsPushProperties(animation_player);
  animation_player.set_element_animations(nullptr);
}

void GroupAnimationPlayer::SetNeedsPushProperties(
    AnimationPlayer& animation_player) {
  needs_push_properties_ = true;

  DCHECK(animation_timeline_);
  animation_timeline_->SetNeedsPushProperties();

  DCHECK(animation_player.element_animations());
  animation_player.element_animations()->SetNeedsPushProperties();
}

void GroupAnimationPlayer::SetAnimationHost(AnimationHost* animation_host) {
  animation_host_ = animation_host;
}

void GroupAnimationPlayer::SetAnimationTimeline(AnimationTimeline* timeline) {
  if (animation_timeline_ == timeline)
    return;

  // We need to unregister player to manage ElementAnimations and observers
  // properly.
  for (auto& animation_player : animation_players_) {
    if (animation_player.element_id() && animation_player.element_animations())
      UnregisterPlayer(animation_player);
  }

  animation_timeline_ = timeline;

  // Register player only if layer AND host attached.
  for (auto& animation_player : animation_players_) {
    if (animation_player.element_id() && animation_host_)
      RegisterPlayer(animation_player);
  }
}

}  // namespace cc
