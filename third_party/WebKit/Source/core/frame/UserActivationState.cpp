// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/UserActivationState.h"

#include "platform/wtf/Time.h"

namespace blink {

constexpr TimeDelta kActivationLifespan = TimeDelta::FromSeconds(10);

void UserActivationState::Activate() {
  has_been_active_ = true;
  is_active_ = true;
  activation_timestamp_ = TimeTicks::Now();
}

void UserActivationState::Clear() {
  has_been_active_ = false;
  is_active_ = false;
}

bool UserActivationState::IsActive() {
  if (is_active_ &&
      (TimeTicks::Now() - activation_timestamp_ > kActivationLifespan)) {
    is_active_ = false;
  }
  return is_active_;
}

bool UserActivationState::ConsumeIfActive() {
  if (!IsActive())
    return false;
  is_active_ = false;
  return true;
}

}  // namespace blink
