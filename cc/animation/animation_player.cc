// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_player.h"

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

scoped_refptr<AnimationPlayer> AnimationPlayer::Create(int id) {
  return base::WrapRefCounted(new AnimationPlayer(id));
}

AnimationPlayer::AnimationPlayer(int id)
    : animation_host_(), animation_timeline_(), animation_delegate_(), id_(id) {
  DCHECK(id_);
}

AnimationPlayer::~AnimationPlayer() {
  DCHECK(!animation_timeline_);
}

scoped_refptr<AnimationPlayer> AnimationPlayer::CreateImplInstance() const {
  scoped_refptr<AnimationPlayer> player = AnimationPlayer::Create(id());
  return player;
}

ElementId AnimationPlayer::element_id() const {
  NOTREACHED();
  return ElementId();
}

ElementId AnimationPlayer::element_id_of_ticker(int ticker_id) const {
  DCHECK(GetTickerById(ticker_id));
  return GetTickerById(ticker_id)->element_id();
}

bool AnimationPlayer::IsElementAttached(ElementId id) const {
  return !!element_to_ticker_id_map_.count(id);
}

void AnimationPlayer::SetAnimationHost(AnimationHost* animation_host) {
  animation_host_ = animation_host;
}

void AnimationPlayer::SetAnimationTimeline(AnimationTimeline* timeline) {
  if (animation_timeline_ == timeline)
    return;

  // We need to unregister player to manage ElementAnimations and observers
  // properly.
  if (!element_to_ticker_id_map_.empty() && animation_host_) {
    // Destroy ElementAnimations or release it if it's still needed.
    UnregisterTicker();
  }

  animation_timeline_ = timeline;

  // Register player only if layer AND host attached. Unlike the
  // SingleAnimationPlayer case, all tickers have been attached to their
  // corresponding elements.
  if (!element_to_ticker_id_map_.empty() && animation_host_) {
    for_each(element_to_ticker_id_map_.begin(), element_to_ticker_id_map_.end(),
             [&](ElementToTickerIdMap::value_type& x) {
               for_each(x.second.begin(), x.second.end(),
                        [&](auto& y) { RegisterTicker(x.first, y); });
             });
  }
}

bool AnimationPlayer::has_element_animations() const {
  return !element_to_ticker_id_map_.empty();
}

scoped_refptr<ElementAnimations> AnimationPlayer::element_animations(
    int ticker_id) const {
  return GetTickerById(ticker_id)->element_animations();
}

void AnimationPlayer::AttachElement(ElementId element_id) {
  NOTREACHED();
}

void AnimationPlayer::AttachElementWithTicker(ElementId element_id,
                                              int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->AttachElement(element_id);
  element_to_ticker_id_map_[element_id].emplace(ticker_id);
  // Register player only if layer AND host attached.
  if (animation_host_) {
    // Create ElementAnimations or re-use existing.
    RegisterTicker(element_id, ticker_id);
  }
}

void AnimationPlayer::DetachElement() {
  if (animation_host_) {
    // Destroy ElementAnimations or release it if it's still needed.
    UnregisterTicker();
  }

  for_each(element_to_ticker_id_map_.begin(), element_to_ticker_id_map_.end(),
           [&](ElementToTickerIdMap::value_type& x) {
             for_each(x.second.begin(), x.second.end(),
                      [&](auto& y) { GetTickerById(y)->DetachElement(); });
           });
  element_to_ticker_id_map_.clear();
}

void AnimationPlayer::RegisterTicker(ElementId element_id, int ticker_id) {
  DCHECK(animation_host_);
  DCHECK(!GetTickerById(ticker_id)->has_bound_element_animations());

  if (!GetTickerById(ticker_id)->has_attached_element())
    return;
  animation_host_->RegisterTickerForElement(element_id,
                                            GetTickerById(ticker_id));
}

void AnimationPlayer::UnregisterTicker() {
  for_each(element_to_ticker_id_map_.begin(), element_to_ticker_id_map_.end(),
           [&](ElementToTickerIdMap::value_type& x) {
             for_each(x.second.begin(), x.second.end(), [&](auto& y) {
               if (GetTickerById(y)->has_attached_element() &&
                   GetTickerById(y)->has_bound_element_animations())
                 animation_host_->UnregisterTickerForElement(x.first,
                                                             GetTickerById(y));
             });
           });
  animation_host_->RemoveFromTicking(this);
}

void AnimationPlayer::PushAttachedTickersToImplThread(
    AnimationPlayer* player_impl) const {
  for (auto& kv : id_to_ticker_map_) {
    auto& ticker = kv.second;
    AnimationTicker* ticker_impl = player_impl->GetTickerById(ticker->id());
    if (ticker_impl)
      continue;

    std::unique_ptr<AnimationTicker> to_add = ticker->CreateImplInstance();
    player_impl->AddTicker(std::move(to_add));
  }
}

void AnimationPlayer::RemoveDetachedTickersFromImplThread(
    AnimationPlayer* player_impl) const {
  IdToTickerMap& tickers_impl = player_impl->id_to_ticker_map_;

  // Erase all the impl tickers which |this| doesn't have.
  for (auto it = tickers_impl.begin(); it != tickers_impl.end();) {
    if (GetTickerById(it->second->id())) {
      ++it;
    } else {
      if (it->second->element_animations()) {
        player_impl->element_to_ticker_id_map_[it->second->element_id()].erase(
            it->second->id());
        player_impl->id_to_ticker_map_.erase(it->second->id());
        it->second->SetAnimationPlayer(nullptr);
        it->second->DetachElement();
      }
      it = tickers_impl.erase(it);
    }
  }
}

void AnimationPlayer::PushPropertiesToImplThread(AnimationPlayer* player_impl) {
  for (auto& kv : id_to_ticker_map_) {
    AnimationTicker* ticker = kv.second.get();
    if (AnimationTicker* ticker_impl =
            player_impl->GetTickerById(ticker->id())) {
      ticker->PushPropertiesTo(ticker_impl);
    }
  }
}

void AnimationPlayer::AddAnimationToTicker(std::unique_ptr<Animation> animation,
                                           int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->AddAnimation(std::move(animation));
}

void AnimationPlayer::PauseAnimationOfTicker(int animation_id,
                                             double time_offset,
                                             int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->PauseAnimation(animation_id, time_offset);
}

void AnimationPlayer::RemoveAnimationFromTicker(int animation_id,
                                                int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->RemoveAnimation(animation_id);
}

void AnimationPlayer::AbortAnimationOfTicker(int animation_id, int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->AbortAnimation(animation_id);
}

void AnimationPlayer::AbortAnimationsOfTicker(
    TargetProperty::Type target_property,
    bool needs_completion,
    int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->AbortAnimations(target_property, needs_completion);
}

void AnimationPlayer::PushPropertiesTo(AnimationPlayer* player_impl) {
  PushAttachedTickersToImplThread(player_impl);
  RemoveDetachedTickersFromImplThread(player_impl);
  PushPropertiesToImplThread(player_impl);
}

void AnimationPlayer::Tick(base::TimeTicks monotonic_time) {
  DCHECK(!monotonic_time.is_null());
  for_each(id_to_ticker_map_.begin(), id_to_ticker_map_.end(),
           [&](auto& x) { x.second->Tick(monotonic_time, nullptr); });
}

void AnimationPlayer::UpdateState(bool start_ready_animations,
                                  AnimationEvents* events) {
  NOTREACHED();
}

void AnimationPlayer::UpdateStateForTicker(bool start_ready_animations,
                                           AnimationEvents* events,
                                           int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->UpdateState(start_ready_animations, events);
  GetTickerById(ticker_id)->UpdateTickingState(UpdateTickingType::NORMAL);
}

void AnimationPlayer::AddToTicking() {
  DCHECK(animation_host_);
  animation_host_->AddToTicking(this);
}

void AnimationPlayer::AnimationRemovedFromTicking() {
  DCHECK(animation_host_);
  animation_host_->RemoveFromTicking(this);
}

void AnimationPlayer::NotifyAnimationStarted(const AnimationEvent& event) {
  if (animation_delegate_) {
    animation_delegate_->NotifyAnimationStarted(
        event.monotonic_time, event.target_property, event.group_id);
  }
}

void AnimationPlayer::NotifyAnimationFinished(const AnimationEvent& event) {
  if (animation_delegate_) {
    animation_delegate_->NotifyAnimationFinished(
        event.monotonic_time, event.target_property, event.group_id);
  }
}

void AnimationPlayer::NotifyAnimationAborted(const AnimationEvent& event) {
  if (animation_delegate_) {
    animation_delegate_->NotifyAnimationAborted(
        event.monotonic_time, event.target_property, event.group_id);
  }
}

void AnimationPlayer::NotifyAnimationTakeover(const AnimationEvent& event) {
  DCHECK(event.target_property == TargetProperty::SCROLL_OFFSET);

  if (animation_delegate_) {
    DCHECK(event.curve);
    std::unique_ptr<AnimationCurve> animation_curve = event.curve->Clone();
    animation_delegate_->NotifyAnimationTakeover(
        event.monotonic_time, event.target_property, event.animation_start_time,
        std::move(animation_curve));
  }
}

size_t AnimationPlayer::TickingAnimationsCount() const {
  NOTREACHED();
  return 0;
}

size_t AnimationPlayer::TickingAnimationsCountOfTicker(int ticker_id) const {
  DCHECK(GetTickerById(ticker_id));
  return GetTickerById(ticker_id)->TickingAnimationsCount();
}

void AnimationPlayer::SetNeedsCommit() {
  DCHECK(animation_host_);
  animation_host_->SetNeedsCommit();
}

void AnimationPlayer::SetNeedsPushProperties() {
  DCHECK(animation_timeline_);
  animation_timeline_->SetNeedsPushProperties();
}

void AnimationPlayer::ActivateAnimations() {
  NOTREACHED();
}

void AnimationPlayer::ActivateAnimationsOfTicker(int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->ActivateAnimations();
  GetTickerById(ticker_id)->UpdateTickingState(UpdateTickingType::NORMAL);
}

Animation* AnimationPlayer::GetAnimationOfTicker(
    TargetProperty::Type target_property,
    int ticker_id) const {
  DCHECK(GetTickerById(ticker_id));
  return GetTickerById(ticker_id)->GetAnimation(target_property);
}

std::string AnimationPlayer::ToString(int ticker_id) const {
  DCHECK(GetTickerById(ticker_id));
  return base::StringPrintf(
      "AnimationPlayer{id=%d, element_id=%s, animations=[%s]}", id_,
      GetTickerById(ticker_id)->element_id().ToString().c_str(),
      GetTickerById(ticker_id)->AnimationsToString().c_str());
}

bool AnimationPlayer::IsWorkletAnimationPlayer() const {
  return false;
}

void AnimationPlayer::AddTicker(std::unique_ptr<AnimationTicker> ticker) {
  int id = ticker->id();
  ticker->SetAnimationPlayer(this);
  id_to_ticker_map_[id] = std::move(ticker);
}

AnimationTicker* AnimationPlayer::animation_ticker() const {
  NOTREACHED();
  return GetTickerById(0);
}

AnimationTicker* AnimationPlayer::GetTickerById(int ticker_id) const {
  return id_to_ticker_map_.find(ticker_id) != id_to_ticker_map_.end()
             ? id_to_ticker_map_.find(ticker_id)->second.get()
             : nullptr;
}

}  // namespace cc
