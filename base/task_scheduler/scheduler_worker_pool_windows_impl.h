// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_WINDOWS_IMPL_H
#define BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_WINDOWS_IMPL_H

#include <Windows.h>

#include "base/base_export.h"
#include "base/logging.h"
#include "base/synchronization/atomic_flag.h"
#include "base/task_scheduler/priority_queue.h"
#include "base/task_scheduler/scheduler_worker_pool.h"

namespace base {
namespace internal {

// A SchedulerWorkerPool implementation backed by the Windows Thread Pool API.
//
// Windows Thread Pool API official documentation:
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686766(v=vs.85).aspx
//
// Blog posts on the Windows Thread Pool API:
// https://msdn.microsoft.com/magazine/hh335066.aspx
// https://msdn.microsoft.com/magazine/hh394144.aspx
// https://msdn.microsoft.com/magazine/hh456398.aspx
// https://msdn.microsoft.com/magazine/hh547107.aspx
// https://msdn.microsoft.com/magazine/hh580731.aspx
class BASE_EXPORT SchedulerWorkerPoolWindowsImpl : public SchedulerWorkerPool {
 public:
  SchedulerWorkerPoolWindowsImpl(TaskTracker* task_tracker,
                                 DelayedTaskManager* delayed_task_manager);

  // Destroying a SchedulerWorkerPoolWindowsImpl is not allowed in
  // production; it is always leaked. In tests, it can only be destroyed after
  // JoinForTesting() has returned.
  ~SchedulerWorkerPoolWindowsImpl() override;

  // Starts the worker pool and allows tasks to begin running.
  void Start();

  // SchedulerWorkerPool:
  void JoinForTesting() override;

 private:
  // Callback that gets run by |pool_|. It runs a task off the next sequence on
  // the |priority_queue_|.
  static void CALLBACK RunNextSequence(PTP_CALLBACK_INSTANCE,
                                       void* scheduler_worker_pool_windows_impl,
                                       PTP_WORK);

  // SchedulerWorkerPool:
  void SubmitSequence(scoped_refptr<Sequence> sequence) override;

  // Returns the top Sequence off the |priority_queue_|. Returns nullptr
  // if the |priority_queue_| is empty.
  scoped_refptr<Sequence> GetWork();

  // Pointer to a thread pool object. Thread pool objects each have their own
  // threads.
  PTP_POOL pool_;

  // Pointer to a work object. This work object executes RunNextSequence and
  // has bounded to it a pointer to the current |SchedulerWorkerPoolWindowsImpl|
  // as well as a pointer to |environment_|.
  PTP_WORK work_;

  // Callback environment. |pool_| is associated with |environment_| so that
  // work objects using this environment run on the correct pool.
  TP_CALLBACK_ENVIRON environment_;

  // PriorityQueue from which all threads of this worker pool get work.
  PriorityQueue priority_queue_;

  // Indicates whether the pool has been started yet. This is only accessed
  // under |priority_queue_|'s lock.
  bool started_ = false;

#if DCHECK_IS_ON()
  // Set once JoinForTesting() has returned.
  AtomicFlag join_for_testing_returned_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SchedulerWorkerPoolWindowsImpl);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCHEDULER_WORKER_POOL_WINDOWS_IMPL_H
