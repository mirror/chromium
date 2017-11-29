// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_TASK_QUEUE_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_TASK_QUEUE_H_

#include "platform/scheduler/base/task_queue.h"

namespace blink {
namespace scheduler {

class WorkerScheduler;

class PLATFORM_EXPORT WorkerTaskQueue : public TaskQueue {
 public:
  enum class QueueType {
    // Keep WorkerTaskQueue::NameForQueueType in sync.
    kControl = 0,
    kDefault = 1,
    kGlobalScope = 2,
    kWorker = 3,
    kCompositor = 4,
    kIdle = 5,
    kTest = 6,
    kCount = 7
  };

  static const char* NameForQueueType(QueueType queue_type);

  WorkerTaskQueue(std::unique_ptr<internal::TaskQueueImpl> impl,
                  const Spec& spec,
                  QueueType queue_type,
                  WorkerScheduler* worker_scheduler);
  ~WorkerTaskQueue() override;

  void OnTaskCompleted(const TaskQueue::Task& task,
                       base::TimeTicks start,
                       base::TimeTicks end);

  QueueType queue_type() const { return queue_type_; }

 private:
  QueueType queue_type_;
  // Not owned.
  WorkerScheduler* worker_scheduler_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WORKER_TASK_QUEUE_H_
