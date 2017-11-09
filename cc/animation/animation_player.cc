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

bool AnimationPlayer::IsElementAttached(ElementId id) const {
  return !!element_to_ticker_map_.count(id);
}

void AnimationPlayer::SetAnimationHost(AnimationHost* animation_host) {
  LOG(ERROR) << "Set host " << animation_host << " for player " << id_;
  animation_host_ = animation_host;
}

void AnimationPlayer::SetAnimationTimeline(AnimationTimeline* timeline) {
  if (animation_timeline_ == timeline)
    return;

  // We need to unregister player to manage ElementAnimations and observers
  // properly.
  if (!element_to_ticker_map_.empty() && animation_host_) {
    // Destroy ElementAnimations or release it if it's still needed.
    for_each(element_to_ticker_map_.begin(), element_to_ticker_map_.end(),
             [&](ElementToTickerMap::value_type& x) {
               // DCHECK_GT(x.second.size(), 0);
               if (x.second.begin()->get()->has_bound_element_animations())
                 animation_host_->UnregisterPlayerForElement(x.first, this);
             });
  }
  //  UnregisterPlayer();

  animation_timeline_ = timeline;
  if (!timeline) {
    element_to_ticker_map_.clear();
  }

  // Register player only if layer AND host attached. Unlike the
  // SingleAnimationPlayer case, all tickers have been  attached to their
  // corresponding elements.
  if (!element_to_ticker_map_.empty() && animation_host_) {
    for_each(element_to_ticker_map_.begin(), element_to_ticker_map_.end(),
             [&](ElementToTickerMap::value_type& x) {
               animation_host_->RegisterPlayerForElement(x.first, this);
             });
  }
}

// bool AnimationPlayer::has_any_animation() const {
//  return GetUniqueTickerForSingleAnimationPlayer()->has_any_animation();
//}

scoped_refptr<ElementAnimations> AnimationPlayer::element_animations() const {
  return GetUniqueTickerForSingleAnimationPlayer()->element_animations();
}

void AnimationPlayer::AttachElement(ElementId element_id) {
  // for_each(animation_tickers_.begin(), animation_tickers_.end(), [&](auto& x)
  // {
  //  x->AttachElement(element_id);
  //  element_to_ticker_map_.emplace(element_id, std::move(x));
  //});
  // animation_tickers_.clear();
  // DebugElementToTickerMap();

  LOG(ERROR) << "AP AttachElement called";
  // FakeMap();
  element_to_ticker_map_[element_id].emplace(base::MakeUnique<AnimationTicker>(
      this, AnimationIdProvider::NextTickerId(), element_id));

  LOG(ERROR) << "map updated";
  // animation_ticker_->AttachElement(element_id);
  // LOG(ERROR) << "AP  " << id_ << " " << animation_ticker_->id() << " " <<
  // element_id;

  // Register player only if layer AND host attached.
  if (animation_host_) {
    LOG(ERROR) << "RegisterPlayerForElement";
    // Create ElementAnimations or re-use existing.
    animation_host_->RegisterPlayerForElement(element_id, this);
    LOG(ERROR) << "RegisterPlayerForElement done";
    LOG(ERROR) << "Ticker: " << GetUniqueTickerForSingleAnimationPlayer()->id()
               << " "
               << element_to_ticker_map_.begin()
                      ->second.begin()
                      ->get()
                      ->has_bound_element_animations();
  }
}

void AnimationPlayer::DetachElement() {
  // DCHECK(animation_ticker_->has_attached_element());

  if (animation_host_) {
    DebugElementToTickerMap();
    // Destroy ElementAnimations or release it if it's still needed.
    for_each(element_to_ticker_map_.begin(), element_to_ticker_map_.end(),
             [&](ElementToTickerMap::value_type& x) {
               animation_host_->UnregisterPlayerForElement(x.first, this);
             });
  }
  //  UnregisterPlayer();

  for_each(element_to_ticker_map_.begin(), element_to_ticker_map_.end(),
           [&](ElementToTickerMap::value_type& x) {
             for_each(x.second.begin(), x.second.end(),
                      [&](auto& y) { y->DetachElement(); });
           });
  //  GetUniqueTickerForSingleAnimationPlayer()->DetachElement();
}

// void AnimationPlayer::RegisterPlayer(ElementId element_id) {
//  DCHECK(animation_host_);
//  DCHECK(animation_ticker_->has_attached_element());
//  DCHECK(!animation_ticker_->has_bound_element_animations());
//
//  // Create ElementAnimations or re-use existing.
//  animation_host_->RegisterPlayerForElement(animation_ticker_->element_id(),
//                                            this);
//}
//
// void AnimationPlayer::UnregisterPlayer() {
//  DCHECK(animation_host_);
//
//  // Destroy ElementAnimations or release it if it's still needed.
//  for_each(element_to_ticker_map_.begin(), element_to_ticker_map_.end(),
//           [&](ElementToTickerMap::value_type& x) {
//             animation_host_->UnregisterPlayerForElement(x.first, this);
//           });
//}

void AnimationPlayer::AddAnimation(std::unique_ptr<Animation> animation) {
  GetUniqueTickerForSingleAnimationPlayer()->AddAnimation(std::move(animation));
}

void AnimationPlayer::PauseAnimation(int animation_id, double time_offset) {
  GetUniqueTickerForSingleAnimationPlayer()->PauseAnimation(animation_id,
                                                            time_offset);
}

void AnimationPlayer::RemoveAnimation(int animation_id) {
  GetUniqueTickerForSingleAnimationPlayer()->RemoveAnimation(animation_id);
}

void AnimationPlayer::AbortAnimation(int animation_id) {
  GetUniqueTickerForSingleAnimationPlayer()->AbortAnimation(animation_id);
}

void AnimationPlayer::AbortAnimations(TargetProperty::Type target_property,
                                      bool needs_completion) {
  GetUniqueTickerForSingleAnimationPlayer()->AbortAnimations(target_property,
                                                             needs_completion);
}

void AnimationPlayer::PushPropertiesTo(AnimationPlayer* player_impl) {
  GetUniqueTickerForSingleAnimationPlayer()->PushPropertiesTo(
      player_impl->GetUniqueTickerForSingleAnimationPlayer());
}

void AnimationPlayer::Tick(base::TimeTicks monotonic_time) {
  DCHECK(!monotonic_time.is_null());
  GetUniqueTickerForSingleAnimationPlayer()->Tick(monotonic_time, nullptr);
}

void AnimationPlayer::UpdateState(bool start_ready_animations,
                                  AnimationEvents* events) {
  GetUniqueTickerForSingleAnimationPlayer()->UpdateState(start_ready_animations,
                                                         events);
  GetUniqueTickerForSingleAnimationPlayer()->UpdateTickingState(
      UpdateTickingType::NORMAL);
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

bool AnimationPlayer::NotifyAnimationFinishedForTesting(
    TargetProperty::Type target_property,
    int group_id) {
  AnimationEvent event(AnimationEvent::FINISHED,
                       GetUniqueTickerForSingleAnimationPlayer()->element_id(),
                       group_id, target_property, base::TimeTicks());
  return GetUniqueTickerForSingleAnimationPlayer()->NotifyAnimationFinished(
      event);
}

size_t AnimationPlayer::TickingAnimationsCount() const {
  return GetUniqueTickerForSingleAnimationPlayer()->TickingAnimationsCount();
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
  GetUniqueTickerForSingleAnimationPlayer()->ActivateAnimations();
  GetUniqueTickerForSingleAnimationPlayer()->UpdateTickingState(
      UpdateTickingType::NORMAL);
}

Animation* AnimationPlayer::GetAnimation(
    TargetProperty::Type target_property) const {
  return GetUniqueTickerForSingleAnimationPlayer()->GetAnimation(
      target_property);
}

std::string AnimationPlayer::ToString() const {
  LOG(ERROR) << "AP ToString called";
  return base::StringPrintf(
      "AnimationPlayer{id=%d, element_id=%s, animations=[%s]}", id_,
      GetUniqueTickerForSingleAnimationPlayer()
          ->element_id()
          .ToString()
          .c_str(),
      GetUniqueTickerForSingleAnimationPlayer()->AnimationsToString().c_str());
}

bool AnimationPlayer::IsWorkletAnimationPlayer() const {
  return false;
}

void AnimationPlayer::AddTicker(std::unique_ptr<AnimationTicker> ticker) {
  animation_ticker_ = std::move(ticker);
  LOG(ERROR) << "Create ticker " << animation_ticker_->id()
             << " and add it to player " << id_;
}

AnimationTicker* AnimationPlayer::animation_ticker() const {
  return animation_ticker_.get();
}

AnimationTicker* AnimationPlayer::GetUniqueTickerForSingleAnimationPlayer()
    const {
  LOG(ERROR) << "map empty? " << element_to_ticker_map_.empty();
  return element_to_ticker_map_.empty()
             ? animation_ticker_.get()
             : element_to_ticker_map_.begin()->second.begin()->get();
}

std::vector<AnimationTicker*> AnimationPlayer::animation_tickers(
    ElementId element_id) {
  std::vector<AnimationTicker*> tickers;
  if (element_to_ticker_map_.empty()) {
    tickers.push_back(animation_ticker_.get());
    return tickers;
  }
  auto iter = element_to_ticker_map_.find(element_id);
  if (iter != element_to_ticker_map_.end()) {
    for_each(iter->second.begin(), iter->second.end(),
             [&](auto& x) { tickers.push_back(x.get()); });
  }
  return tickers;
}

void AnimationPlayer::FakeMap() {
  element_to_ticker_map_[ElementId(100)].emplace(
      base::MakeUnique<AnimationTicker>(this,
                                        AnimationIdProvider::NextTickerId()));
  element_to_ticker_map_[ElementId(100)].emplace(
      base::MakeUnique<AnimationTicker>(this,
                                        AnimationIdProvider::NextTickerId()));
  element_to_ticker_map_[ElementId(200)].emplace(
      base::MakeUnique<AnimationTicker>(this,
                                        AnimationIdProvider::NextTickerId()));
}

void AnimationPlayer::DebugElementToTickerMap() {
  for_each(element_to_ticker_map_.begin(), element_to_ticker_map_.end(),
           [](ElementToTickerMap::value_type& x) {
             LOG(ERROR) << "id: " << x.first.ToString()
                        << " ticker: " << x.second.size();
           });
}

}  // namespace cc
