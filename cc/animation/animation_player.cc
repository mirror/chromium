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

  // We need to unregister ticker to manage ElementAnimations and observers
  // properly.
  if (!element_to_ticker_id_map_.empty() && animation_host_) {
    // Destroy ElementAnimations or release it if it's still needed.
    UnregisterTickers();
  }

  animation_timeline_ = timeline;

  // Register player only if layer AND host attached. Unlike the
  // SingleAnimationPlayer case, all tickers have been attached to their
  // corresponding elements.
  if (!element_to_ticker_id_map_.empty() && animation_host_) {
    RegisterTickers();
  }
}

bool AnimationPlayer::has_element_animations() const {
  return !element_to_ticker_id_map_.empty();
}

scoped_refptr<ElementAnimations> AnimationPlayer::element_animations(
    int ticker_id) const {
  return GetTickerById(ticker_id)->element_animations();
}

void AnimationPlayer::AttachElementForTicker(ElementId element_id,
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

void AnimationPlayer::DetachElementForTicker(ElementId element_id,
                                             int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  DCHECK_EQ(GetTickerById(ticker_id)->element_id(), element_id);

  UnregisterTicker(element_id, ticker_id);
  GetTickerById(ticker_id)->DetachElement();
  element_to_ticker_id_map_[element_id].erase(ticker_id);
}

void AnimationPlayer::DetachElement() {
  if (animation_host_) {
    // Destroy ElementAnimations or release it if it's still needed.
    UnregisterTickers();
  }

  for (auto pair = element_to_ticker_id_map_.begin();
       pair != element_to_ticker_id_map_.end();) {
    for (auto ticker = pair->second.begin(); ticker != pair->second.end();) {
      GetTickerById(*ticker)->DetachElement();
      ticker = pair->second.erase(ticker);
    }
    pair = element_to_ticker_id_map_.erase(pair);
  }
  DCHECK_EQ(element_to_ticker_id_map_.size(), 0u);
}

void AnimationPlayer::RegisterTicker(ElementId element_id, int ticker_id) {
  DCHECK(animation_host_);
  AnimationTicker* ticker = GetTickerById(ticker_id);
  DCHECK(!ticker->has_bound_element_animations());

  if (!ticker->has_attached_element())
    return;
  animation_host_->RegisterTickerForElement(element_id, ticker);
}

void AnimationPlayer::UnregisterTicker(ElementId element_id, int ticker_id) {
  DCHECK(animation_host_);
  AnimationTicker* ticker = GetTickerById(ticker_id);
  DCHECK(ticker);
  if (ticker->has_attached_element() &&
      ticker->has_bound_element_animations()) {
    animation_host_->UnregisterTickerForElement(element_id, ticker);
  }
}
void AnimationPlayer::RegisterTickers() {
  for (auto& element_id_ticker_id : element_to_ticker_id_map_) {
    const ElementId element_id = element_id_ticker_id.first;
    const std::unordered_set<int>& ticker_ids = element_id_ticker_id.second;
    for (auto& ticker_id : ticker_ids)
      RegisterTicker(element_id, ticker_id);
  }
}

void AnimationPlayer::UnregisterTickers() {
  for (auto& element_id_ticker_id : element_to_ticker_id_map_) {
    const ElementId element_id = element_id_ticker_id.first;
    const std::unordered_set<int>& ticker_ids = element_id_ticker_id.second;
    for (auto& ticker_id : ticker_ids)
      UnregisterTicker(element_id, ticker_id);
  }
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

  std::vector<int> tickers_to_remove;
  // Erase all the impl tickers which |this| doesn't have.
  for (auto& id_ticker_pair : tickers_impl) {
    const int ticker_id = id_ticker_pair.first;
    const AnimationTicker* ticker = id_ticker_pair.second.get();
    if (!GetTickerById(ticker_id)) {
      if (ticker->element_animations()) {
        player_impl->DetachElementForTicker(ticker->element_id(), ticker_id);
      }
      tickers_to_remove.push_back(ticker_id);
    }
  }
  for (auto& ticker_id : tickers_to_remove)
    player_impl->RemoveTicker(ticker_id);
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

void AnimationPlayer::AddAnimationForTicker(
    std::unique_ptr<Animation> animation,
    int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->AddAnimation(std::move(animation));
}

void AnimationPlayer::PauseAnimationForTicker(int animation_id,
                                              double time_offset,
                                              int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->PauseAnimation(animation_id, time_offset);
}

void AnimationPlayer::RemoveAnimationForTicker(int animation_id,
                                               int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->RemoveAnimation(animation_id);
}

void AnimationPlayer::AbortAnimationForTicker(int animation_id, int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->AbortAnimation(animation_id);
}

void AnimationPlayer::AbortAnimationsForTicker(
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
  for (auto& id_ticker_pair : id_to_ticker_map_)
    id_ticker_pair.second->Tick(monotonic_time, nullptr);
}

void AnimationPlayer::UpdateState(bool start_ready_animations,
                                  AnimationEvents* events) {
  for (auto& id_ticker_pair : id_to_ticker_map_)
    UpdateStateForTicker(start_ready_animations, events, id_ticker_pair.first);
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
  int count = 0;
  for (auto& id_ticker_pair : id_to_ticker_map_)
    count += id_ticker_pair.second->TickingAnimationsCount();
  return count;
}

void AnimationPlayer::SetNeedsCommit() {
  DCHECK(animation_host_);
  animation_host_->SetNeedsCommit();
}

void AnimationPlayer::SetNeedsPushProperties() {
  if (!animation_timeline_)
    return;
  animation_timeline_->SetNeedsPushProperties();
}

void AnimationPlayer::ActivateAnimations() {
  for (auto& id_ticker_pair : id_to_ticker_map_) {
    AnimationTicker* ticker = id_ticker_pair.second.get();
    ticker->ActivateAnimations();
    ticker->UpdateTickingState(UpdateTickingType::NORMAL);
  }
}

Animation* AnimationPlayer::GetAnimationForTicker(
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
  DCHECK_NE(id, 0);
  ticker->SetAnimationPlayer(this);
  id_to_ticker_map_[id] = std::move(ticker);

  SetNeedsPushProperties();
}

void AnimationPlayer::RemoveTicker(int ticker_id) {
  DCHECK(GetTickerById(ticker_id));
  GetTickerById(ticker_id)->SetAnimationPlayer(nullptr);
  id_to_ticker_map_.erase(ticker_id);

  SetNeedsPushProperties();
}

AnimationTicker* AnimationPlayer::GetTickerById(int ticker_id) const {
  const auto it = id_to_ticker_map_.find(ticker_id);
  return it != id_to_ticker_map_.end() ? it->second.get() : nullptr;
}

}  // namespace cc
