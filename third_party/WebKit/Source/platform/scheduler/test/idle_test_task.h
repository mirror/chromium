// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_IDLE_TEST_TASK_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_IDLE_TEST_TASK_H_

#include <vector>
#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"

namespace blink {
namespace scheduler {

class SingleThreadIdleTaskRunner;

void IdleTestTask(int* run_count,
                  base::TimeTicks* deadline_out,
                  base::TimeTicks deadline);

void RepostingUpdateClockIdleTestTask(
    SingleThreadIdleTaskRunner* idle_task_runner,
    int* run_count,
    base::SimpleTestTickClock* clock,
    base::TimeDelta advance_time,
    std::vector<base::TimeTicks>* deadlines,
    base::TimeTicks deadline);

void UpdateClockIdleTestTask(base::SimpleTestTickClock* clock,
                             int* run_count,
                             base::TimeTicks set_time,
                             base::TimeTicks deadline);

void UpdateClockToDeadlineIdleTestTask(base::SimpleTestTickClock* clock,
                                       int* run_count,
                                       base::TimeTicks deadline);

extern int g_max_idle_task_reposts;

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_TEST_IDLE_TEST_TASK_H_
