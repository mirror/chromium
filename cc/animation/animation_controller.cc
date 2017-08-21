// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/animation_controller.h"

#include <inttypes.h>
#include <algorithm>

#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_timeline.h"

namespace cc {

AnimationController::AnimationController(int id)
    : animation_host_(), animation_timeline_(), id_(id) {
  DCHECK(id_);
}

AnimationController::AnimationController() {}

AnimationController::~AnimationController() {
  // DCHECK(!animation_timeline_);
}

void AnimationController::SetAnimationHost(AnimationHost* animation_host) {
  animation_host_ = animation_host;
}

void AnimationController::SetAnimationTimeline(AnimationTimeline* timeline) {
  if (animation_timeline_ == timeline)
    return;

  //  // We need to unregister player to manage ElementAnimations and observers
  //  // properly.
  //  if (element_id_ && element_animations_)
  //    UnregisterPlayer();
  //
  //  animation_timeline_ = timeline;
  //
  //  // Register player only if layer AND host attached.
  //  if (element_id_ && animation_host_)
  //    RegisterPlayer();
}

void AnimationController::AddEffectController(
    std::unique_ptr<EffectController> effect_controller) {
  DCHECK(effect_controller->id());
  effect_controllers_.insert(
      std::make_pair(effect_controller->id(), std::move(effect_controller)));

  // SetNeedsPushProperties(); ?
}

void AnimationController::RemoveEffectController(
    std::unique_ptr<EffectController> effect_controller) {
  DCHECK(effect_controller->id());
  effect_controllers_.erase(effect_controller->id());
}
// void AnimationController::AttachElement(ElementId element_id) {
//  DCHECK(!element_id_);
//  DCHECK(element_id);
//
//  element_id_ = element_id;
//
//  // Register player only if layer AND host attached.
//  if (animation_host_)
//    RegisterPlayer();
//}
//
// void AnimationController::DetachElement() {
//  DCHECK(element_id_);
//
//  if (animation_host_)
//    UnregisterPlayer();
//
//  element_id_ = ElementId();
//}
//
// void AnimationController::RegisterPlayer() {
//  DCHECK(element_id_);
//  DCHECK(animation_host_);
//  DCHECK(!element_animations_);
//
//  // Create ElementAnimations or re-use existing.
//  animation_host_->RegisterPlayerForElement(element_id_, this);
//  // Get local reference to shared ElementAnimations.
//  BindElementAnimations();
//}
//
// void AnimationController::UnregisterPlayer() {
//  DCHECK(element_id_);
//  DCHECK(animation_host_);
//  DCHECK(element_animations_);
//
//  UnbindElementAnimations();
//  // Destroy ElementAnimations or release it if it's still needed.
//  animation_host_->UnregisterPlayerForElement(element_id_, this);
//}
//
// void AnimationController::BindElementAnimations() {
//  DCHECK(!element_animations_);
//  element_animations_ =
//      animation_host_->GetElementAnimationsForElementId(element_id_);
//  DCHECK(element_animations_);
//
//  if (!animations_.empty())
//    AnimationAdded();
//
//  SetNeedsPushProperties();
//}
//
// void AnimationController::UnbindElementAnimations() {
//  SetNeedsPushProperties();
//  element_animations_ = nullptr;
//}

}  // namespace cc
