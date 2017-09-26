// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_LOG_SOURCE_ACCESS_RATE_LIMITER_H_
#define EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_LOG_SOURCE_ACCESS_RATE_LIMITER_H_

#include <algorithm>
#include <memory>

#include "base/macros.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"

namespace extensions {

// Keeps track of how frequently a log source may be accessed.
// - Keeps count of the number of immediate accesses allowed (the number of
//   times the source can be accessed immediately without delay).
// - Keeps track of when the last access was. Will recharge the number of
//   allowed immediate accesses over time.
//
// This is a generic bookkeeping class that does not handle the log source
// itself.
class LogSourceAccessRateLimiter {
 public:
  LogSourceAccessRateLimiter(size_t max_num_accesses,
                             const base::TimeDelta& recharge_period);
  ~LogSourceAccessRateLimiter();

  // Override the default base::Time clock with a custom clock for testing.
  // Pass in |clock|=nullptr to revert to default behavior. Does not take
  // ownership of |clock|.
  void SetTickClockForTesting(base::TickClock* clock) {
    tick_clock_ = clock ? clock : &default_tick_clock_;
  }

  // Attempt to access the source. Returns true if the source could be accessed,
  // or false if not.
  bool TryAccess();

 private:
  // |num_accesses_| may only be increased up to this limit.
  const size_t max_num_accesses_;

  // When the log source was last accessed successfully.
  base::TimeTicks last_access_time_;

  // The time it takes to increment |num_accesses_| by 1. If this is set to 0,
  // accesses are unlimited.
  const base::TimeDelta recharge_period_;

  // Accumulates the time elapsed. When this exceeds |recharge_period_|, it
  // can be decremented by |recharge_period_| to increase |num_accesses_|. Even
  // if this causes |num_accesses_| to exceed |max_num_accesses_|, it must be
  // reset to 0, while |num_accesses_| gets capped at |max_num_accesses_|.
  base::TimeDelta recharge_counter_;

  // Points to a timer clock implementation for keeping track of access times.
  // Points to |default_tick_clock_| by default, but can override it for
  // testing.
  base::TickClock* tick_clock_;

  // Default timer clock.
  base::DefaultTickClock default_tick_clock_;

  DISALLOW_COPY_AND_ASSIGN(LogSourceAccessRateLimiter);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_FEEDBACK_PRIVATE_LOG_SOURCE_ACCESS_RATE_LIMITER_H_
