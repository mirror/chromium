// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UserActivationState_h
#define UserActivationState_h

#include "platform/wtf/Time.h"

namespace blink {

class CORE_EXPORT UserActivationState {
 public:
  UserActivationState() : has_been_active_(false), is_active_(false) {}

  void activate() {
    has_been_active_ = true;
    is_active_ = true;
    activation_timestamp_ = TimeTicks::Now();
  }

  void clear() {
    has_been_active_ = false;
    is_active_ = false;
  }

  bool hasBeenActive() const { return has_been_active_; }

  bool isActive() {
    if (is_active_ &&
        (activation_timestamp_ + activation_lifetime_ > TimeTicks::Now())) {
      is_active_ = false;
    }
    return is_active_;
  }

  bool consumeIfActive() {
    if (!isActive())
      return false;
    is_active_ = false;
    return true;
  }

 private:
  bool has_been_active_;
  bool is_active_;
  TimeTicks activation_timestamp_;
  const TimeDelta activation_lifetime_ = TimeDelta::FromSeconds(10);
};

}  // namespace blink

#endif  // UserActivationState_h
