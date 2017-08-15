// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_H_
#define BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_H_

#include <memory>

#include "base/base_export.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner.h"
#include "base/task_scheduler/sequence.h"
#include "base/task_scheduler/task.h"
#include "base/task_scheduler/task_traits.h"

namespace base {
namespace internal {

class BASE_EXPORT TaskTracker;
class BASE_EXPORT DelayedTaskManager;

// Interface for a worker pool.
class BASE_EXPORT SchedulerWorkerPool {
 public:
  virtual ~SchedulerWorkerPool() = default;

  // Returns a TaskRunner whose PostTask invocations result in scheduling tasks
  // in this SchedulerWorkerPool using |traits|. Tasks may run in any order and
  // in parallel.
  virtual scoped_refptr<TaskRunner> CreateTaskRunnerWithTraits(
      const TaskTraits& traits);

  // Returns a SequencedTaskRunner whose PostTask invocations result in
  // scheduling tasks in this SchedulerWorkerPool using |traits|. Tasks run one
  // at a time in posting order.
  virtual scoped_refptr<SequencedTaskRunner>
  CreateSequencedTaskRunnerWithTraits(const TaskTraits& traits);

  // Posts |task| to be executed by this SchedulerWorkerPool as part of
  // |sequence|. |task| won't be executed before its delayed run time, if any.
  // Returns true if |task| is posted.
  virtual bool PostTaskWithSequence(std::unique_ptr<Task> task,
                                    scoped_refptr<Sequence> sequence);

  // Posts |task| to be executed by this SchedulerWorkerPool as part of
  // |sequence|. This must only be called after |task| has gone through
  // PostTaskWithSequence() and after |task|'s delayed run time.
  virtual void PostTaskWithSequenceNow(std::unique_ptr<Task> task,
                                       scoped_refptr<Sequence> sequence) = 0;

  static void SetCurrentWorkerPool(
      const SchedulerWorkerPool* scheduler_worker_pool);

  static const SchedulerWorkerPool* GetCurrentWorkerPool();

 protected:
  SchedulerWorkerPool(TaskTracker* task_tracker,
                      DelayedTaskManager* delayed_task_manager);

  TaskTracker* const task_tracker_;
  DelayedTaskManager* const delayed_task_manager_;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_H_
