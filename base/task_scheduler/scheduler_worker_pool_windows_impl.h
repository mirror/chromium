// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_WINDOWS_IMPL_H
#define BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_WINDOWS_IMPL_H

#include "Windows.h"

#include "base/base_export.h"
#include "base/logging.h"
#include "base/synchronization/atomic_flag.h"
#include "base/task_scheduler/priority_queue.h"
#include "base/task_scheduler/scheduler_worker_pool.h"

namespace base {
namespace internal {

class BASE_EXPORT SchedulerWorkerPoolWindowsImpl : public SchedulerWorkerPool {
 public:
  SchedulerWorkerPoolWindowsImpl(TaskTracker* task_tracker,
                                 DelayedTaskManager* delayed_task_manager);

  ~SchedulerWorkerPoolWindowsImpl() override;

  // Starts the worker pool and allows work to begin running.
  void Start();

  // SchedulerWorkerPool:
  void PostTaskWithSequenceNow(std::unique_ptr<Task> task,
                               scoped_refptr<Sequence> sequence) override;

 private:
  // Callback that gets run by |pool_|. It runs a task off the next sequence on
  // the |shared_priority_queue_|.
  static void CALLBACK RunNextSequence(PTP_CALLBACK_INSTANCE,
                                       void* scheduler_worker_pool_windows_impl,
                                       PTP_WORK);

  // Returns the top Sequence off the |shared_priority_queue_|. Returns nullptr
  // if the |shared_priority_queue_| is empty.
  scoped_refptr<Sequence> GetWork();

  // Adds |sequence| to the |shared_priority_queue_|.
  void EnqueueSequence(scoped_refptr<Sequence> sequence);

  // Submits a task to the Windows thread pool that performs the
  // |RunNextSequence| callback if Start() has been called. No-op if Start()
  // hasn't been called.
  void SubmitToWindowsThreadPool();

  PTP_POOL pool_;
  PTP_WORK work_;
  TP_CALLBACK_ENVIRON environment_;

  // PriorityQueue from which all threads of this worker pool get work.
  PriorityQueue shared_priority_queue_;

  // Flag that indicates whether the pool has been started yet.
  AtomicFlag started_;
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_WINDOWS_IMPL_H
