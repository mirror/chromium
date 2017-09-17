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

class ScopedTaskEnvironment::TestTaskTracker
    : public internal::TaskSchedulerImpl::TaskTrackerImpl {
 public:
  TestTaskTracker();

  // Allow running tasks.
  void AllowRunRask();

  // Disallow running tasks. No-ops and returns false if a task is running.
  bool DisallowRunTasks();

 private:
  friend class ScopedTaskEnvironment;

  // internal::TaskSchedulerImpl::TaskTrackerImpl:
  void RunOrSkipTask(std::unique_ptr<internal::Task> task,
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
      message_loop_(main_thread_type == MainThreadType::MOCK_TIME
                        ? nullptr
                        : (std::make_unique<MessageLoop>(
                              main_thread_type == MainThreadType::DEFAULT
                                  ? MessageLoop::TYPE_DEFAULT
                                  : (main_thread_type == MainThreadType::UI
                                         ? MessageLoop::TYPE_UI
                                         : MessageLoop::TYPE_IO)))),
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
  task_tracker_->AllowRunRask();
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
  do {
    task_tracker_->AllowRunRask();

    // The logic below would also be correct without this first
    // RunLoop().RunUntilIdle(). However its presence allows main thread tasks
    // to run in parallel with ongoing TaskScheduler tasks, increasing
    // likelihood of TSAN catching threading errors.
    RunLoop().RunUntilIdle();

    // Flush any remaining TaskScheduler work.
    TaskScheduler::GetInstance()->FlushForTesting();

    // Flush any remaining tasks on main thread (which must have been posted by
    // the last few TaskScheduler tasks above). Disallow running TaskScheduler
    // tasks while doing this: this prevents an ABA problem with the
    // GetNumIncompleteUndelayedTasksForTesting() test coming up (which could
    // otherwise be zero despite work having occured after the end of this
    // RunUntilIdle()).
    // Note: DisallowRunTasks() fails when a TaskScheduler task is running. A
    // TaskScheduler task may be running after the TaskScheduler queue became
    // empty even if no tasks ran on the main thread in these cases:
    //  - An undelayed task became ripe for execution.
    //  - A task was posted from an external thread.
    // In either case, loop again.
    if (!task_tracker_->DisallowRunTasks())
      continue;
    RunLoop().RunUntilIdle();

    // Loop again if the last few tasks on the main thread posted anything back
    // to TaskScheduler.
  } while (task_tracker_->GetNumIncompleteUndelayedTasksForTesting() != 0);

  if (execution_control_mode_ != ExecutionMode::QUEUED)
    task_tracker_->AllowRunRask();
}

void ScopedTaskEnvironment::FastForwardBy(TimeDelta delta) {
  DCHECK(mock_time_task_runner_);
  mock_time_task_runner_->FastForwardBy(delta);
}

void ScopedTaskEnvironment::FastForwardUntilNoTasksRemain() {
  DCHECK(mock_time_task_runner_);
  mock_time_task_runner_->FastForwardUntilNoTasksRemain();
}

ScopedTaskEnvironment::TestTaskTracker::TestTaskTracker()
    : can_run_tasks_cv_(&lock_) {}

void ScopedTaskEnvironment::TestTaskTracker::AllowRunRask() {
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
    std::unique_ptr<internal::Task> task,
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
