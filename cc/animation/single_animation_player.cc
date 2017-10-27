// Copyright 2015 The Chromium Authors. All rights reserved.
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
  //      animation_host_(),
  //      animation_timeline_(),
  //      animation_delegate_(),
  //      id_(id),
  //      animation_ticker_(new AnimationTicker(this)) {
  AnimationPlayer::AddTicker(base::MakeUnique<AnimationTicker>(this));
  //  DCHECK(id_);
}

SingleAnimationPlayer::~SingleAnimationPlayer() {
  //  DCHECK(!animation_timeline_);
}

//// scoped_refptr<SingleAnimationPlayer>
/// SingleAnimationPlayer::CreateImplInstance() const { /
/// scoped_refptr<SingleAnimationPlayer> player =
/// SingleAnimationPlayer::Create(id()); /  return player;
////}
//
// ElementId SingleAnimationPlayer::element_id() const {
//  return GetUniqueTickerForSingleAnimationPlayer()->element_id();
//}
//
// void SingleAnimationPlayer::SetAnimationHost(AnimationHost* animation_host) {
//  animation_host_ = animation_host;
//}
//
// void SingleAnimationPlayer::SetAnimationTimeline(AnimationTimeline* timeline)
// {
//  if (animation_timeline_ == timeline)
//    return;
//
//  // We need to unregister player to manage ElementAnimations and observers
//  // properly.
//  if (animation_ticker_->has_attached_element() &&
//      animation_ticker_->has_bound_element_animations())
//    UnregisterPlayer();
//
//  animation_timeline_ = timeline;
//
//  // Register player only if layer AND host attached.
//  if (animation_ticker_->has_attached_element() && animation_host_)
//    RegisterPlayer();
//}
//
// bool SingleAnimationPlayer::has_any_animation() const {
//  return animation_ticker_->has_any_animation();
//}
//
//// scoped_refptr<ElementAnimations>
/// SingleAnimationPlayer::element_animations()
/// const { /  return animation_ticker_->element_animations();
////}
//
// void SingleAnimationPlayer::AttachElement(ElementId element_id) {
//  GetUniqueTickerForSingleAnimationPlayer()->AttachElement(element_id);
//
//  // Register player only if layer AND host attached.
//  if (animation_host())
//    RegisterPlayer();
//}
//
// void SingleAnimationPlayer::DetachElement() {
//  DCHECK(GetUniqueTickerForSingleAnimationPlayer()->has_attached_element());
//
//  if (animation_host())
//    UnregisterPlayer();
//
//  GetUniqueTickerForSingleAnimationPlayer()->DetachElement();
//}
//
// void SingleAnimationPlayer::RegisterPlayer() {
//   DCHECK(animation_host());
//   DCHECK(GetUniqueTickerForSingleAnimationPlayer()->has_attached_element());
//   DCHECK(!GetUniqueTickerForSingleAnimationPlayer()
//               ->has_bound_element_animations());
//
//   // Create ElementAnimations or re-use existing.
//   animation_host()->RegisterPlayerForElement(
//       GetUniqueTickerForSingleAnimationPlayer()->element_id(), this);
// }
//
// void SingleAnimationPlayer::UnregisterPlayer() {
//   DCHECK(animation_host());
//   DCHECK(GetUniqueTickerForSingleAnimationPlayer()->has_attached_element());
//   DCHECK(GetUniqueTickerForSingleAnimationPlayer()
//              ->has_bound_element_animations());
//
//   // Destroy ElementAnimations or release it if it's still needed.
//   animation_host()->UnregisterPlayerForElement(
//       GetUniqueTickerForSingleAnimationPlayer()->element_id(), this);
// }
//
// void SingleAnimationPlayer::AddAnimation(std::unique_ptr<Animation>
// animation) {
//  animation_ticker_->AddAnimation(std::move(animation));
//}
//
// void SingleAnimationPlayer::PauseAnimation(int animation_id, double
// time_offset) {
//  animation_ticker_->PauseAnimation(animation_id, time_offset);
//}
//
// void SingleAnimationPlayer::RemoveAnimation(int animation_id) {
//  animation_ticker_->RemoveAnimation(animation_id);
//}
//
// void SingleAnimationPlayer::AbortAnimation(int animation_id) {
//  animation_ticker_->AbortAnimation(animation_id);
//}
//
// void SingleAnimationPlayer::AbortAnimations(TargetProperty::Type
// target_property,
//                                      bool needs_completion) {
//  animation_ticker_->AbortAnimations(target_property, needs_completion);
//}
//
//// void SingleAnimationPlayer::PushPropertiesTo(SingleAnimationPlayer*
/// player_impl) { /
/// animation_ticker_->PushPropertiesTo(player_impl->animation_ticker_.get());
////}
//
//// void SingleAnimationPlayer::Tick(base::TimeTicks monotonic_time) {
////  DCHECK(!monotonic_time.is_null());
////  animation_ticker_->Tick(monotonic_time, nullptr);
////}
//
// void SingleAnimationPlayer::UpdateState(bool start_ready_animations,
//                                  AnimationEvents* events) {
//  animation_ticker_->UpdateState(start_ready_animations, events);
//  animation_ticker_->UpdateTickingState(UpdateTickingType::NORMAL);
//}
//
// void SingleAnimationPlayer::AddToTicking() {
//  DCHECK(animation_host_);
//  animation_host_->AddToTicking(this);
//}
//
// void SingleAnimationPlayer::AnimationRemovedFromTicking() {
//  DCHECK(animation_host_);
//  animation_host_->RemoveFromTicking(this);
//}
//
// void SingleAnimationPlayer::NotifyAnimationStarted(const AnimationEvent&
// event) {
//  if (animation_delegate_) {
//    animation_delegate_->NotifyAnimationStarted(
//        event.monotonic_time, event.target_property, event.group_id);
//  }
//}
//
// void SingleAnimationPlayer::NotifyAnimationFinished(const AnimationEvent&
// event) {
//  if (animation_delegate_) {
//    animation_delegate_->NotifyAnimationFinished(
//        event.monotonic_time, event.target_property, event.group_id);
//  }
//}
//
// void SingleAnimationPlayer::NotifyAnimationAborted(const AnimationEvent&
// event) {
//  if (animation_delegate_) {
//    animation_delegate_->NotifyAnimationAborted(
//        event.monotonic_time, event.target_property, event.group_id);
//  }
//}
//
// void SingleAnimationPlayer::NotifyAnimationTakeover(const AnimationEvent&
// event) {
//  DCHECK(event.target_property == TargetProperty::SCROLL_OFFSET);
//
//  if (animation_delegate_) {
//    DCHECK(event.curve);
//    std::unique_ptr<AnimationCurve> animation_curve = event.curve->Clone();
//    animation_delegate_->NotifyAnimationTakeover(
//        event.monotonic_time, event.target_property,
//        event.animation_start_time, std::move(animation_curve));
//  }
//}
//
// bool SingleAnimationPlayer::NotifyAnimationFinishedForTesting(
//    TargetProperty::Type target_property,
//    int group_id) {
//  AnimationEvent event(AnimationEvent::FINISHED,
//                       animation_ticker_->element_id(), group_id,
//                       target_property, base::TimeTicks());
//  return animation_ticker_->NotifyAnimationFinished(event);
//}
//
// void SingleAnimationPlayer::SetNeedsCommit() {
//  DCHECK(animation_host_);
//  animation_host_->SetNeedsCommit();
//}
//
// void SingleAnimationPlayer::SetNeedsPushProperties() {
//  DCHECK(animation_timeline_);
//  animation_timeline_->SetNeedsPushProperties();
//}
//
// void SingleAnimationPlayer::ActivateAnimations() {
//  animation_ticker_->ActivateAnimations();
//  animation_ticker_->UpdateTickingState(UpdateTickingType::NORMAL);
//}
//
// Animation* SingleAnimationPlayer::GetAnimation(
//    TargetProperty::Type target_property) const {
//  return animation_ticker_->GetAnimation(target_property);
//}
//
// std::string SingleAnimationPlayer::ToString() const {
//  return base::StringPrintf(
//      "SingleAnimationPlayer{id=%d, element_id=%s, animations=[%s]}", id_,
//      animation_ticker_->element_id().ToString().c_str(),
//      animation_ticker_->AnimationsToString().c_str());
//}
//
//// bool SingleAnimationPlayer::IsWorkletAnimationPlayer() const {
////  return false;
////}

}  // namespace cc
