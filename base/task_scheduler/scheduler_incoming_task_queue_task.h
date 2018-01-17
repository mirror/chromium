// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/task_scheduler/task.h"

namespace base {
namespace internal {

struct SchedulerIncomingTaskQueueTask : public Task {
  SchedulerIncomingTaskQueueTask(const Location& posted_from,
                                 OnceClosure task,
                                 const TaskTraits& traits,
                                 TimeDelta delay);

  SchedulerIncomingTaskQueueTask(
      SchedulerIncomingTaskQueueTask&& other) noexcept;

  ~SchedulerIncomingTaskQueueTask();

  SchedulerIncomingTaskQueueTask& operator=(
      SchedulerIncomingTaskQueueTask&& other);

  bool task_tracker_aware;

 private:
  DISALLOW_COPY_AND_ASSIGN(SchedulerIncomingTaskQueueTask);
};

}  // namespace internal
}  // namespace base
