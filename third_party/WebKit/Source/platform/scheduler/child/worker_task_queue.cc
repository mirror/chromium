// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/worker_task_queue.h"

#include "platform/scheduler/base/task_queue_impl.h"
#include "platform/scheduler/child/worker_scheduler.h"

namespace blink {
namespace scheduler {

const char* WorkerTaskQueue::NameForQueueType(QueueType queue_type) {
  switch (queue_type) {
    case WorkerTaskQueue::QueueType::kControl:
      return "ControlWTQ";
    case WorkerTaskQueue::QueueType::kDefault:
      return "DefaultWTQ";
    case WorkerTaskQueue::QueueType::kGlobalScope:
      return "GlobalScopeWTQ";
    case WorkerTaskQueue::QueueType::kWorker:
      return "WorkerWTQ";
    case WorkerTaskQueue::QueueType::kCompositor:
      return "CompositorWTQ";
    case WorkerTaskQueue::QueueType::kIdle:
      return "IdleWTQ";
    case WorkerTaskQueue::QueueType::kTest:
      return "TestWTQ";
    case WorkerTaskQueue::QueueType::kCount:
      NOTREACHED();
      return nullptr;
  }
  NOTREACHED();
  return nullptr;
}

WorkerTaskQueue::WorkerTaskQueue(std::unique_ptr<internal::TaskQueueImpl> impl,
                                 const TaskQueue::Spec& spec,
                                 QueueType queue_type,
                                 WorkerScheduler* worker_scheduler)
    : TaskQueue(std::move(impl), spec),
      queue_type_(queue_type),
      worker_scheduler_(worker_scheduler) {
  if (GetTaskQueueImpl()) {
    // TaskQueueImpl may be null for tests.
    GetTaskQueueImpl()->SetOnTaskCompletedHandler(
        base::Bind(&WorkerTaskQueue::OnTaskCompleted, base::Unretained(this)));
  }
}

WorkerTaskQueue::~WorkerTaskQueue() {}

void WorkerTaskQueue::OnTaskCompleted(const TaskQueue::Task& task,
                                      base::TimeTicks start,
                                      base::TimeTicks end) {
  // |worker_scheduler_| can be nullptr in tests.
  if (worker_scheduler_)
    worker_scheduler_->OnTaskCompleted(this, task, start, end);
}

}  // namespace scheduler
}  // namespace blink
