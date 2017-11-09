// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_TIME_CONVERTER_H_
#define BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_TIME_CONVERTER_H_

#include "base/time/time.h"

namespace base {


// TODO(scheduler-dev): Remove conversions when Blink starts using
// base::TimeTicks instead of doubles for time.
static inline base::TimeTicks MonotonicTimeInSecondsToTimeTicks(
    double monotonic_time_in_seconds) {
  return base::TimeTicks() +
         base::TimeDelta::FromSecondsD(monotonic_time_in_seconds);
}


}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_TIME_CONVERTER_H_
