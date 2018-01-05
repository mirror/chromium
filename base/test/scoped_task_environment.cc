// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_task_environment.h"

#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/task_scheduler/task_scheduler_impl.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"

namespace base {
namespace test {

namespace {

std::unique_ptr<MessageLoop> CreateMessageLoopForMainThreadType(
    ScopedTaskEnvironment::MainThreadType main_thread_type) {
  switch (main_thread_type) {
    case ScopedTaskEnvironment::MainThreadType::DEFAULT:
      return std::make_unique<MessageLoop>(MessageLoop::TYPE_DEFAULT);
    case ScopedTaskEnvironment::MainThreadType::MOCK_TIME:
      return nullptr;
    case ScopedTaskEnvironment::MainThreadType::UI:
      return std::make_unique<MessageLoop>(MessageLoop::TYPE_UI);
    case ScopedTaskEnvironment::MainThreadType::IO:
      return std::make_unique<MessageLoop>(MessageLoop::TYPE_IO);
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace

class ScopedTaskEnvironment::TestTaskTracker
    : public internal::TaskSchedulerImpl::TaskTrackerImpl {
 public:
  TestTaskTracker();

  // Allow running tasks.
  void AllowRunTasks();

  // Disallow running tasks. Returns true on success; success requires there to
  // be no tasks currently running.
  bool DisallowRunTasks();

 private:
  friend class ScopedTaskEnvironment;

  // internal::TaskSchedulerImpl::TaskTrackerImpl:
  void RunOrSkipTask(internal::Task task,
                     internal::Sequence* sequence,
                     bool can_run_task) override;

  // Synchronizes accesses to members below.
  Lock lock_;

  // True if running tasks is allowed.
  bool can_run_tasks_ = true;

  // Signaled when |can_run_tasks_| becomes true.
  ConditionVariable can_run_tasks_cv_;

  // Number of tasks that are currently running.
  int num_tasks_running_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestTaskTracker);
};

ScopedTaskEnvironment::ScopedTaskEnvironment(
    MainThreadType main_thread_type,
    ExecutionMode execution_control_mode)
    : execution_control_mode_(execution_control_mode),
      message_loop_(CreateMessageLoopForMainThreadType(main_thread_type)),
      mock_time_task_runner_(
          main_thread_type == MainThreadType::MOCK_TIME
              ? MakeRefCounted<TestMockTimeTaskRunner>(
                    TestMockTimeTaskRunner::Type::kBoundToThread)
              : nullptr),
      task_tracker_(new TestTaskTracker()) {
  CHECK(!TaskScheduler::GetInstance());

  // Instantiate a TaskScheduler with 2 threads in each of its 4 pools. Threads
  // stay alive even when they don't have work.
  // Each pool uses two threads to prevent deadlocks in unit tests that have a
  // sequence that uses WithBaseSyncPrimitives() to wait on the result of
  // another sequence. This isn't perfect (doesn't solve wait chains) but solves
  // the basic use case for now.
  // TODO(fdoray/jeffreyhe): Make the TaskScheduler dynamically replace blocked
  // threads and get rid of this limitation. http://crbug.com/738104
  constexpr int kMaxThreads = 2;
  const TimeDelta kSuggestedReclaimTime = TimeDelta::Max();
  const SchedulerWorkerPoolParams worker_pool_params(kMaxThreads,
                                                     kSuggestedReclaimTime);
  TaskScheduler::SetInstance(std::make_unique<internal::TaskSchedulerImpl>(
      "ScopedTaskEnvironment", WrapUnique(task_tracker_)));
  task_scheduler_ = TaskScheduler::GetInstance();
  TaskScheduler::GetInstance()->Start({worker_pool_params, worker_pool_params,
                                       worker_pool_params, worker_pool_params});

  if (execution_control_mode_ == ExecutionMode::QUEUED)
    CHECK(task_tracker_->DisallowRunTasks());
}

ScopedTaskEnvironment::~ScopedTaskEnvironment() {
  // Ideally this would RunLoop().RunUntilIdle() here to catch any errors or
  // infinite post loop in the remaining work but this isn't possible right now
  // because base::~MessageLoop() didn't use to do this and adding it here would
  // make the migration away from MessageLoop that much harder.
  CHECK_EQ(TaskScheduler::GetInstance(), task_scheduler_);
  // Without FlushForTesting(), DeleteSoon() and ReleaseSoon() tasks could be
  // skipped, resulting in memory leaks.
  task_tracker_->AllowRunTasks();
  TaskScheduler::GetInstance()->FlushForTesting();
  TaskScheduler::GetInstance()->Shutdown();
  TaskScheduler::GetInstance()->JoinForTesting();
  TaskScheduler::SetInstance(nullptr);
}

scoped_refptr<base::SingleThreadTaskRunner>
ScopedTaskEnvironment::GetMainThreadTaskRunner() {
  if (message_loop_)
    return message_loop_->task_runner();
  DCHECK(mock_time_task_runner_);
  return mock_time_task_runner_;
}

void ScopedTaskEnvironment::RunUntilIdle() {
  if (execution_control_mode_ == ExecutionMode::QUEUED)
    task_tracker_->AllowRunTasks();

  // Tasks in |task_tracker_| are running in parallel with this check. It is
  // non-racy however because :
  //   A) The main thread is running this very check and hence not also
  //      processing tasks in parallel.
  //   B) HasIncompleteUndelayedTasksForTesting() is atomic and once it returns
  //      false it can only be made true again by a task on the main thread.
  //   C) Any task posted to the main thread from a completed |task_tracker_|
  //      task has an happens-before relationship with
  //      HasIncompleteUndelayedTasksForTesting() returning false and is
  //      therefore guaranteed to make HasMainThreadTasks() return true if it
  //      wasn't yet processed.
  // Note: HasIncompleteUndelayedTasksForTesting() has to be checked before
  // HasMainThreadTasks() to take advantage of (C).
  // The last part of this check re-disallows running tasks in QUEUED
  // ExecutionMode once idle. Failing to do so means a delayed task became ripe
  // and started running right after determining idleness which breaks the
  // condition and forces this loop to keep going.
  while (task_tracker_->HasIncompleteUndelayedTasksForTesting() ||
         HasMainThreadTasks() ||
         (execution_control_mode_ == ExecutionMode::QUEUED &&
          !task_tracker_->DisallowRunTasks())) {
    base::RunLoop().RunUntilIdle();
    // Avoid busy-looping.
    if (task_tracker_->HasIncompleteUndelayedTasksForTesting()) {
      // TODO(gab): Consider a shorter timeout/better solution
      // (https://crbug.com/789756).
      PlatformThread::Sleep(TimeDelta::FromMilliseconds(1));
    }
  }
}

void ScopedTaskEnvironment::FastForwardBy(TimeDelta delta) {
  DCHECK(mock_time_task_runner_);
  mock_time_task_runner_->FastForwardBy(delta);
}

void ScopedTaskEnvironment::FastForwardUntilNoTasksRemain() {
  DCHECK(mock_time_task_runner_);
  mock_time_task_runner_->FastForwardUntilNoTasksRemain();
}

bool ScopedTaskEnvironment::HasMainThreadTasks() {
  if (message_loop_)
    return !message_loop_->IsIdleForTesting();
  DCHECK(mock_time_task_runner_);
  return mock_time_task_runner_->NextPendingTaskDelay().is_zero();
}

ScopedTaskEnvironment::TestTaskTracker::TestTaskTracker()
    : can_run_tasks_cv_(&lock_) {}

void ScopedTaskEnvironment::TestTaskTracker::AllowRunTasks() {
  AutoLock auto_lock(lock_);
  can_run_tasks_ = true;
  can_run_tasks_cv_.Broadcast();
}

bool ScopedTaskEnvironment::TestTaskTracker::DisallowRunTasks() {
  AutoLock auto_lock(lock_);

  // Can't disallow run task if there are tasks running.
  if (num_tasks_running_ > 0)
    return false;

  can_run_tasks_ = false;
  return true;
}

void ScopedTaskEnvironment::TestTaskTracker::RunOrSkipTask(
    internal::Task task,
    internal::Sequence* sequence,
    bool can_run_task) {
  {
    AutoLock auto_lock(lock_);

    while (!can_run_tasks_)
      can_run_tasks_cv_.Wait();

    ++num_tasks_running_;
  }

  internal::TaskSchedulerImpl::TaskTrackerImpl::RunOrSkipTask(
      std::move(task), sequence, can_run_task);

  {
    AutoLock auto_lock(lock_);

    CHECK_GT(num_tasks_running_, 0);
    CHECK(can_run_tasks_);

    --num_tasks_running_;
  }
}

}  // namespace test
}  // namespace base
