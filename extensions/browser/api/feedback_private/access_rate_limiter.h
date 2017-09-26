// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_ACCESS_RATE_LIMITER_H_
#define EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_ACCESS_RATE_LIMITER_H_

#include <algorithm>
#include <memory>

#include "base/macros.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"

namespace extensions {

// Keeps track of how frequently a resource may be accessed.
// - Keeps count of the number of immediate accesses allowed (the number of
//   times the source can be accessed immediately without delay).
// - Keeps track of when the last access was. Will recharge the number of
//   allowed immediate accesses over time.
class AccessRateLimiter {
 public:
  AccessRateLimiter(size_t max_num_accesses,
                             const base::TimeDelta& recharge_period);
  ~AccessRateLimiter();

  // Override the default base::Time clock with a custom clock for testing.
  // Pass in |clock|=nullptr to revert to default behavior. Does not take
  // ownership of |clock|.
  void SetTickClockForTesting(base::TickClock* clock) {
    tick_clock_ = clock ? clock : &default_tick_clock_;
  }

  // Attempt to access a resource. Returns true if there are available accesses,
  // or false if not. Will update the number of available accesses based on how
  // much time has elapsed since the last access attempt.
  bool TryAccess();

 private:
  // Can only accrue up to this many available accesses.
  const size_t max_num_accesses_;

  // Time when TryAccess() was last called.
  base::TimeTicks last_access_time_;

  // The time it takes to accumulate one extra available access. If this is set
  // 0, accesses are unlimited.
  const base::TimeDelta recharge_period_;

  // Keeps track of how many available accesses there are. There is one for each
  // unit of |recharge_period_|.
  base::TimeDelta recharge_counter_;

  // Points to a timer clock implementation for keeping track of access times.
  // Points to |default_tick_clock_| by default, but can override it for
  // testing.
  base::TickClock* tick_clock_;

  // Default timer clock.
  base::DefaultTickClock default_tick_clock_;

  DISALLOW_COPY_AND_ASSIGN(AccessRateLimiter);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_ACCESS_RATE_LIMITER_H_
