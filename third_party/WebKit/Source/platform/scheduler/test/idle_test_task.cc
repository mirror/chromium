// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/test/idle_test_task.h"

#include "public/platform/scheduler/child/single_thread_idle_task_runner.h"

namespace blink {
namespace scheduler {

int g_max_idle_task_reposts = 2;

void IdleTestTask(int* run_count,
                  base::TimeTicks* deadline_out,
                  base::TimeTicks deadline) {
  (*run_count)++;
  *deadline_out = deadline;
}

void RepostingUpdateClockIdleTestTask(
    SingleThreadIdleTaskRunner* idle_task_runner,
    int* run_count,
    base::SimpleTestTickClock* clock,
    base::TimeDelta advance_time,
    std::vector<base::TimeTicks>* deadlines,
    base::TimeTicks deadline) {
  if ((*run_count + 1) < g_max_idle_task_reposts) {
    idle_task_runner->PostIdleTask(
        FROM_HERE, base::Bind(&RepostingUpdateClockIdleTestTask,
                              base::Unretained(idle_task_runner), run_count,
                              clock, advance_time, deadlines));
  }
  deadlines->push_back(deadline);
  (*run_count)++;
  clock->Advance(advance_time);
}

void UpdateClockIdleTestTask(base::SimpleTestTickClock* clock,
                             int* run_count,
                             base::TimeTicks set_time,
                             base::TimeTicks deadline) {
  clock->Advance(set_time - clock->NowTicks());
  (*run_count)++;
}

void UpdateClockToDeadlineIdleTestTask(base::SimpleTestTickClock* clock,
                                       int* run_count,
                                       base::TimeTicks deadline) {
  UpdateClockIdleTestTask(clock, run_count, deadline, deadline);
}

}  // namespace scheduler
}  // namespace blink
