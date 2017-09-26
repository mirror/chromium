// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/feedback_private/access_rate_limiter.h"

#include "base/time/default_tick_clock.h"

namespace extensions {

AccessRateLimiter::AccessRateLimiter(
    size_t max_num_accesses,
    const base::TimeDelta& recharge_period)
    : max_num_accesses_(max_num_accesses),
      recharge_period_(recharge_period),
      recharge_counter_(recharge_period * max_num_accesses),
      tick_clock_(&default_tick_clock_) {}

AccessRateLimiter::~AccessRateLimiter() {}

bool AccessRateLimiter::TryAccess() {
  // If recharge period is 0, accesses are unlimited.
  if (recharge_period_.is_zero())
    return true;

  if (last_access_time_.is_null())
    last_access_time_ = tick_clock_->NowTicks();

  // First, attempt to recharge.
  const base::TimeTicks now = tick_clock_->NowTicks();
  recharge_counter_ += now - last_access_time_;
  last_access_time_ = now;

  // Did the charge max out?
  if (recharge_counter_ > recharge_period_ * max_num_accesses_)
    recharge_counter_ = recharge_period_ * max_num_accesses_;

  // Is an immediate access available?
  if (recharge_counter_ < recharge_period_)
    return false;

  recharge_counter_ -= recharge_period_;
  return true;
}

}  // namespace extensions
