// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scheduler_incoming_task_queue_task.h"

namespace base {
namespace internal {

SchedulerIncomingTaskQueueTask::SchedulerIncomingTaskQueueTask(
    const Location& posted_from,
    OnceClosure task,
    const TaskTraits& traits,
    TimeDelta delay)
    : Task(posted_from, std::move(task), traits, delay),
      task_tracker_aware(false) {}

SchedulerIncomingTaskQueueTask::SchedulerIncomingTaskQueueTask(
    SchedulerIncomingTaskQueueTask&& other) noexcept
    : Task(std::move(other)), task_tracker_aware(other.task_tracker_aware) {
  other.task_tracker_aware = false;
}

SchedulerIncomingTaskQueueTask::~SchedulerIncomingTaskQueueTask() = default;

SchedulerIncomingTaskQueueTask& SchedulerIncomingTaskQueueTask::operator=(
    SchedulerIncomingTaskQueueTask&& other) {
  Task::operator=(std::move(other));
  task_tracker_aware = other.task_tracker_aware;
  other.task_tracker_aware = false;
  return *this;
}

}  // namespace internal
}  // namespace base
