// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UserActivationState_h
#define UserActivationState_h

#include "platform/wtf/Time.h"

namespace blink {

constexpr TimeDelta kActivationLifespan = TimeDelta::FromSeconds(10);

class CORE_EXPORT UserActivationState {
 public:
  UserActivationState() : has_been_active_(false), is_active_(false) {}

  void Activate() {
    has_been_active_ = true;
    is_active_ = true;
    activation_timestamp_ = TimeTicks::Now();
  }

  void Clear() {
    has_been_active_ = false;
    is_active_ = false;
  }

  // Returns the sticky activation state, which is |true| if
  // |UserActivationState| has ever seen an activation.
  bool HasBeenActive() const { return has_been_active_; }

  // Returns the transient activation state, which is |true| if
  // |UserActivationState| has recently been activated and the transient state
  // hasn't been consumed yet.
  bool IsActive() {
    if (is_active_ &&
        (TimeTicks::Now() - activation_timestamp_ > kActivationLifespan)) {
      is_active_ = false;
    }
    return is_active_;
  }

  // Consumes the transient activation state if available, and returns |true| if
  // successfully consumed.
  bool ConsumeIfActive() {
    if (!IsActive())
      return false;
    is_active_ = false;
    return true;
  }

 private:
  bool has_been_active_;
  bool is_active_;
  TimeTicks activation_timestamp_;
};

}  // namespace blink

#endif  // UserActivationState_h
