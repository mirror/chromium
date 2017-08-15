// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scheduler_worker_pool_windows_impl.h"

#include "base/task_scheduler/task_tracker.h"

namespace base {
namespace internal {

SchedulerWorkerPoolWindowsImpl::SchedulerWorkerPoolWindowsImpl(
    TaskTracker* task_tracker,
    DelayedTaskManager* delayed_task_manager)
    : SchedulerWorkerPool(task_tracker, delayed_task_manager) {}

SchedulerWorkerPoolWindowsImpl::~SchedulerWorkerPoolWindowsImpl() {
  // Finish existing work so that it does not try to de-reference an invalid
  // pointer to this class.
  WaitForThreadpoolWorkCallbacks(work_, false);

  DestroyThreadpoolEnvironment(&environment_);
  CloseThreadpoolWork(work_);
  CloseThreadpool(pool_);
}

void SchedulerWorkerPoolWindowsImpl::Start() {
  DCHECK(!started_.IsSet());

  InitializeThreadpoolEnvironment(&environment_);

  pool_ = CreateThreadpool(nullptr);
  work_ = CreateThreadpoolWork(RunNextSequence, this, &environment_);
  DCHECK(pool_);
  DCHECK(work_);
  SetThreadpoolCallbackPool(&environment_, pool_);

  started_.Set();
  std::unique_ptr<PriorityQueue::Transaction> shared_transaction(
      shared_priority_queue_.BeginTransaction());

  for (size_t i = 0; i < shared_transaction->Size(); ++i) {
    SubmitToWindowsThreadPool();
  }
}

void SchedulerWorkerPoolWindowsImpl::PostTaskWithSequenceNow(
    std::unique_ptr<Task> task,
    scoped_refptr<Sequence> sequence) {
  DCHECK(task);
  DCHECK(sequence);

  // Confirm that |task| is ready to run (its delayed run time is either null or
  // in the past).
  DCHECK_LE(task->delayed_run_time, TimeTicks::Now());

  const bool sequence_was_empty = sequence->PushTask(std::move(task));
  if (sequence_was_empty) {
    // Insert |sequence| in |shared_priority_queue_| if it was empty before
    // |task| was inserted into it. Otherwise, one of these must be true:
    // - |sequence| is already in a PriorityQueue, or,
    // - A worker is running a Task from |sequence|. It will insert |sequence|
    //   in a PriorityQueue once it's done running the Task.
    EnqueueSequence(sequence);
    // Post a task to the Windows thread pool.
    SubmitToWindowsThreadPool();
  }
}

void CALLBACK SchedulerWorkerPoolWindowsImpl::RunNextSequence(
    PTP_CALLBACK_INSTANCE,
    void* scheduler_worker_pool_windows_impl,
    PTP_WORK) {
  SchedulerWorkerPoolWindowsImpl* worker_pool =
      static_cast<SchedulerWorkerPoolWindowsImpl*>(
          scheduler_worker_pool_windows_impl);

  SchedulerWorkerPool::SetCurrentWorkerPool(worker_pool);

  scoped_refptr<Sequence> sequence = worker_pool->GetWork();
  DCHECK(sequence);

  const bool sequence_became_empty =
      worker_pool->task_tracker_->RunNextTask(sequence.get());

  // ReEnque sequence then submit another task to the Windows thread pool.
  if (!sequence_became_empty) {
    worker_pool->EnqueueSequence(std::move(sequence));
    worker_pool->SubmitToWindowsThreadPool();
  }
}

scoped_refptr<Sequence> SchedulerWorkerPoolWindowsImpl::GetWork() {
  scoped_refptr<Sequence> sequence;
  {
    std::unique_ptr<PriorityQueue::Transaction> shared_transaction(
        shared_priority_queue_.BeginTransaction());

    if (shared_transaction->IsEmpty())
      return nullptr;
    else
      sequence = shared_transaction->PopSequence();
  }

  DCHECK(sequence);

  return sequence;
}

void SchedulerWorkerPoolWindowsImpl::EnqueueSequence(
    scoped_refptr<Sequence> sequence) {
  const SequenceSortKey sequence_sort_key = sequence->GetSortKey();
  shared_priority_queue_.BeginTransaction()->Push(std::move(sequence),
                                                  sequence_sort_key);
}

void SchedulerWorkerPoolWindowsImpl::SubmitToWindowsThreadPool() {
  if (started_.IsSet())
    SubmitThreadpoolWork(work_);
}

}  // namespace internal
}  // namespace base
