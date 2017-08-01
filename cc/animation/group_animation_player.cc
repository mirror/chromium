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

GroupAnimationPlayer::GroupAnimationPlayer(int id)
    : animation_host_(),
      animation_timeline_(),
      animation_delegate_(),
      id_(id),
      needs_push_properties_(false),
      needs_to_start_animations_(false) {
  // is_ticking_(false),
  // scroll_offset_animation_was_interrupted_(false) {
  DCHECK(id_);
}

GroupAnimationPlayer::~GroupAnimationPlayer() {}

scoped_refptr<GroupAnimationPlayer> GroupAnimationPlayer::CreateImplInstance()
    const {
  scoped_refptr<GroupAnimationPlayer> player =
      GroupAnimationPlayer::Create(id());
  return player;
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
    if (animation_player->element_id() &&
        animation_player->element_animations())
      UnregisterPlayer(*animation_player);
  }

  animation_timeline_ = timeline;

  // Register player only if layer AND host attached.
  for (auto& animation_player : animation_players_) {
    if (animation_player->element_id() && animation_host_)
      RegisterPlayer(*animation_player);
  }
}

void GroupAnimationPlayer::DetachPlayer() {
  for (auto& animation_player : animation_players_)
    animation_timeline_->DetachPlayer(animation_player);
}

void GroupAnimationPlayer::DetachElement() {
  for (auto& animation_player : animation_players_) {
    if (animation_player->element_animations())
      animation_player->DetachElement();
  }
}

void GroupAnimationPlayer::RegisterPlayer(AnimationPlayer& animation_player) {
  DCHECK(animation_host_);

  animation_host_->RegisterPlayerForElement(animation_player.element_id(),
                                            &animation_player);
  // Get local reference to shared ElementAnimations.
  BindElementAnimations(animation_player);
}

void GroupAnimationPlayer::UnregisterPlayer(AnimationPlayer& animation_player) {
  DCHECK(animation_host_);

  // Destroy ElementAnimations or release it if it's still needed.
  UnbindElementAnimations(animation_player);
  animation_host_->UnregisterPlayerForElement(animation_player.element_id(),
                                              &animation_player);
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

void GroupAnimationPlayer::Tick(base::TimeTicks monotonic_time) {
  DCHECK(!monotonic_time.is_null());

  for (auto& animation_player : animation_players_) {
    animation_player->Tick(monotonic_time);
  }
}

void GroupAnimationPlayer::UpdateState(bool start_ready_animations,
                                       AnimationEvents* events) {
  for (auto& animation_player : animation_players_) {
    animation_player->UpdateState(start_ready_animations, events);
  }
}

bool GroupAnimationPlayer::NotifyAnimationStarted(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);

  for (auto& animation_player : animation_players_) {
    if (animation_player->NotifyAnimationStarted(event))
      return true;
  }

  return false;
}

bool GroupAnimationPlayer::NotifyAnimationFinished(
    const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);

  for (auto& animation_player : animation_players_) {
    if (animation_player->NotifyAnimationFinished(event))
      return true;
  }

  return false;
}

bool GroupAnimationPlayer::NotifyAnimationFinishedForTesting(
    TargetProperty::Type target_property,
    int group_id) {
  for (auto& animation_player : animation_players_) {
    if (animation_player->NotifyAnimationFinishedForTesting(target_property,
                                                            group_id))
      return true;
  }

  return false;
}

bool GroupAnimationPlayer::NotifyAnimationAborted(const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);

  for (auto& animation_player : animation_players_) {
    if (animation_player->NotifyAnimationAborted(event))
      return true;
  }

  return false;
}

void GroupAnimationPlayer::NotifyAnimationTakeover(
    const AnimationEvent& event) {
  DCHECK(!event.is_impl_only);
  DCHECK(event.target_property == TargetProperty::SCROLL_OFFSET);

  // We need to purge animations marked for deletion on CT.
  for (auto& animation_player : animation_players_) {
    SetNeedsPushProperties(*animation_player);
  }

  if (animation_delegate_) {
    DCHECK(event.curve);
    std::unique_ptr<AnimationCurve> animation_curve = event.curve->Clone();
    animation_delegate_->NotifyAnimationTakeover(
        event.monotonic_time, event.target_property, event.animation_start_time,
        std::move(animation_curve));
  }
}

void GroupAnimationPlayer::AddToTicking(scoped_refptr<AnimationPlayer> player) {
  DCHECK(std::find(ticking_players_.begin(), ticking_players_.end(), player) ==
         ticking_players_.end());
  ticking_players_.push_back(player);
}

void GroupAnimationPlayer::RemoveFromTicking(
    scoped_refptr<AnimationPlayer> player) {
  auto to_erase =
      std::find(ticking_players_.begin(), ticking_players_.end(), player);
  if (to_erase != ticking_players_.end())
    ticking_players_.erase(to_erase);
}

}  // namespace cc
