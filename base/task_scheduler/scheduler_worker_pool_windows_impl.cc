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
#if DCHECK_IS_ON()
  // Verify join_for_testing has been called to ensure that there is no more
  // outstanding work. Otherwise, work may try to de-reference an invalid
  // pointer to this class.
  DCHECK(join_for_testing_returned_.IsSet());
#endif
  DestroyThreadpoolEnvironment(&environment_);
  CloseThreadpoolWork(work_);
  CloseThreadpool(pool_);
}

void SchedulerWorkerPoolWindowsImpl::Start() {
  InitializeThreadpoolEnvironment(&environment_);

  pool_ = CreateThreadpool(nullptr);
  work_ = CreateThreadpoolWork(RunNextSequence, this, &environment_);
  DCHECK(pool_);
  DCHECK(work_);
  SetThreadpoolCallbackPool(&environment_, pool_);

  size_t local_num_sequences_before_start;
  {
    std::unique_ptr<PriorityQueue::Transaction> transaction(
        priority_queue_.BeginTransaction());
    DCHECK(!started_);
    started_ = true;
    local_num_sequences_before_start = transaction->Size();
  }

  // Schedule sequences added to |priority_queue_| before Start().
  for (size_t i = 0; i < local_num_sequences_before_start; ++i)
    SubmitThreadpoolWork(work_);
}

void SchedulerWorkerPoolWindowsImpl::JoinForTesting() {
  WaitForThreadpoolWorkCallbacks(work_, true);
#if DCHECK_IS_ON()
  DCHECK(!join_for_testing_returned_.IsSet());
  join_for_testing_returned_.Set();
#endif
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

  // Re-enqueue sequence and then submit another task to the Windows thread
  // pool.
  if (!sequence_became_empty)
    worker_pool->SubmitSequence(std::move(sequence));

  SchedulerWorkerPool::ResetCurrentWorkerPool();
}

scoped_refptr<Sequence> SchedulerWorkerPoolWindowsImpl::GetWork() {
  std::unique_ptr<PriorityQueue::Transaction> transaction(
      priority_queue_.BeginTransaction());
  if (transaction->IsEmpty())
    return nullptr;
  return transaction->PopSequence();
}

void SchedulerWorkerPoolWindowsImpl::SubmitSequence(
    scoped_refptr<Sequence> sequence) {
  const SequenceSortKey sequence_sort_key = sequence->GetSortKey();
  std::unique_ptr<PriorityQueue::Transaction> transaction(
      priority_queue_.BeginTransaction());

  transaction->Push(std::move(sequence), sequence_sort_key);
  if (started_)
    SubmitThreadpoolWork(work_);
}

}  // namespace internal
}  // namespace base
