// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scheduler_worker_pool.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "base/task_scheduler/delayed_task_manager.h"
#include "base/task_scheduler/scheduler_worker_pool_impl.h"
#include "base/task_scheduler/scheduler_worker_pool_params.h"
#include "base/task_scheduler/scheduler_worker_pool_windows_impl.h"
#include "base/task_scheduler/task_tracker.h"
#include "base/task_scheduler/test_task_factory.h"
#include "base/task_scheduler/test_utils.h"
#include "base/test/test_timeouts.h"
#include "base/threading/simple_thread.h"
#include "base/threading/thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

namespace {

constexpr size_t kNumWorkersInWorkerPool = 4;
constexpr size_t kNumThreadsPostingTasks = 4;
constexpr size_t kNumTasksPostedPerThread = 150;

enum class PoolType { GENERIC, WINDOWS };

struct PoolExecutionType {
  PoolType pool_type_;
  test::ExecutionMode execution_mode_;
};

using PostNestedTask = test::TestTaskFactory::PostNestedTask;

class ThreadPostingTasks : public SimpleThread {
 public:
  // Constructs a thread that posts |num_tasks_posted_per_thread| tasks to
  // |worker_pool| through an |execution_mode| task runner. If
  // |wait_before_post_task| is WAIT_FOR_ALL_WORKERS_IDLE, the thread waits
  // until all undelayed tasks are complete before posting a new task. If
  // |post_nested_task| is YES, each task posted by this thread posts another
  // task when it runs.
  ThreadPostingTasks(SchedulerWorkerPool* worker_pool,
                     test::ExecutionMode execution_mode,
                     PostNestedTask post_nested_task)
      : SimpleThread("ThreadPostingTasks"),
        worker_pool_(worker_pool),
        post_nested_task_(post_nested_task),
        factory_(test::CreateTaskRunnerWithExecutionMode(worker_pool,
                                                         execution_mode),
                 execution_mode) {
    DCHECK(worker_pool_);
  }

  const test::TestTaskFactory* factory() const { return &factory_; }

 private:
  void Run() override {
    EXPECT_FALSE(factory_.task_runner()->RunsTasksInCurrentSequence());

    for (size_t i = 0; i < kNumTasksPostedPerThread; ++i) {
      EXPECT_TRUE(factory_.PostTask(post_nested_task_, Closure()));
    }
  }

  SchedulerWorkerPool* const worker_pool_;
  const scoped_refptr<TaskRunner> task_runner_;
  const PostNestedTask post_nested_task_;
  test::TestTaskFactory factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadPostingTasks);
};

class TaskSchedulerWorkerPoolTest
    : public testing::TestWithParam<PoolExecutionType> {
 protected:
  TaskSchedulerWorkerPoolTest()
      : service_thread_("TaskSchedulerServiceThread") {}

  void SetUp() override {
    CreateStartWorkerPoolWithType(GetParam().pool_type_);
  }

  void TearDown() override {
    service_thread_.Stop();
    if (GetParam().pool_type_ == PoolType::GENERIC) {
      SchedulerWorkerPoolImpl* pool_impl =
          static_cast<SchedulerWorkerPoolImpl*>(worker_pool_.get());
      pool_impl->JoinForTesting();
    }
  }

  std::unique_ptr<SchedulerWorkerPool> worker_pool_;

  TaskTracker task_tracker_;

 private:
  void CreateStartWorkerPoolWithType(PoolType type) {
    ASSERT_FALSE(worker_pool_);
    service_thread_.Start();
    delayed_task_manager_.Start(service_thread_.task_runner());

    switch (type) {
      case PoolType::GENERIC: {
        std::unique_ptr<SchedulerWorkerPoolImpl> scheduler_worker_pool_impl =
            MakeUnique<SchedulerWorkerPoolImpl>(
                "TestWorkerPool", ThreadPriority::NORMAL, &task_tracker_,
                &delayed_task_manager_);
        scheduler_worker_pool_impl->Start(SchedulerWorkerPoolParams(
            kNumWorkersInWorkerPool, TimeDelta::Max()));
        worker_pool_ = std::move(scheduler_worker_pool_impl);

        break;
      }
      case PoolType::WINDOWS: {
        std::unique_ptr<SchedulerWorkerPoolWindowsImpl>
            scheduler_worker_pool_win =
                MakeUnique<SchedulerWorkerPoolWindowsImpl>(
                    &task_tracker_, &delayed_task_manager_);
        scheduler_worker_pool_win->Start();
        worker_pool_ = std::move(scheduler_worker_pool_win);
        break;
      }
    }
    ASSERT_TRUE(worker_pool_);
  }

  Thread service_thread_;
  DelayedTaskManager delayed_task_manager_;

  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerWorkerPoolTest);
};

void ShouldNotRun() {
  ADD_FAILURE() << "Ran a task that shouldn't run.";
}

}  // namespace

TEST_P(TaskSchedulerWorkerPoolTest, PostTasks) {
  // Create threads to post tasks.
  std::vector<std::unique_ptr<ThreadPostingTasks>> threads_posting_tasks;
  for (size_t i = 0; i < kNumThreadsPostingTasks; ++i) {
    threads_posting_tasks.push_back(MakeUnique<ThreadPostingTasks>(
        worker_pool_.get(), GetParam().execution_mode_, PostNestedTask::NO));
    threads_posting_tasks.back()->Start();
  }

  // Wait for all tasks to run.
  for (const auto& thread_posting_tasks : threads_posting_tasks) {
    thread_posting_tasks->Join();
    thread_posting_tasks->factory()->WaitForAllTasksToRun();
  }

  // Flush the task tracker to be sure that no task accesses its TestTaskFactory
  // after |thread_posting_tasks| is destroyed.
  task_tracker_.Flush();
}

TEST_P(TaskSchedulerWorkerPoolTest, NestedPostTasks) {
  // Create threads to post tasks. Each task posted by these threads will post
  // another task when it runs.
  std::vector<std::unique_ptr<ThreadPostingTasks>> threads_posting_tasks;
  for (size_t i = 0; i < kNumThreadsPostingTasks; ++i) {
    threads_posting_tasks.push_back(MakeUnique<ThreadPostingTasks>(
        worker_pool_.get(), GetParam().execution_mode_, PostNestedTask::YES));
    threads_posting_tasks.back()->Start();
  }

  // Wait for all tasks to run.
  for (const auto& thread_posting_tasks : threads_posting_tasks) {
    thread_posting_tasks->Join();
    thread_posting_tasks->factory()->WaitForAllTasksToRun();
  }

  // Flush the task tracker to be sure that no task accesses its TestTaskFactory
  // after |thread_posting_tasks| is destroyed.
  task_tracker_.Flush();
}

// Verify that a Task can't be posted after shutdown.
TEST_P(TaskSchedulerWorkerPoolTest, PostTaskAfterShutdown) {
  auto task_runner = test::CreateTaskRunnerWithExecutionMode(
      worker_pool_.get(), GetParam().execution_mode_);
  task_tracker_.Shutdown();
  EXPECT_FALSE(task_runner->PostTask(FROM_HERE, BindOnce(&ShouldNotRun)));
}

// Verify that a Task runs shortly after its delay expires.
TEST_P(TaskSchedulerWorkerPoolTest, PostDelayedTask) {
  TimeTicks start_time = TimeTicks::Now();

  // Post a task with a short delay.
  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  EXPECT_TRUE(test::CreateTaskRunnerWithExecutionMode(
                  worker_pool_.get(), GetParam().execution_mode_)
                  ->PostDelayedTask(
                      FROM_HERE,
                      BindOnce(&WaitableEvent::Signal, Unretained(&task_ran)),
                      TestTimeouts::tiny_timeout()));

  // Wait until the task runs.
  task_ran.Wait();

  // Expect the task to run after its delay expires, but not more than 250 ms
  // after that.
  const TimeDelta actual_delay = TimeTicks::Now() - start_time;
  EXPECT_GE(actual_delay, TestTimeouts::tiny_timeout());
  EXPECT_LT(actual_delay,
            TimeDelta::FromMilliseconds(250) + TestTimeouts::tiny_timeout());
}

// Verify that the RunsTasksInCurrentSequence() method of a SEQUENCED TaskRunner
// returns false when called from a task that isn't part of the sequence. Note:
// Tests that use TestTaskFactory already verify that
// RunsTasksInCurrentSequence() returns true when appropriate so this method
// complements it to get full coverage of that method.
TEST_P(TaskSchedulerWorkerPoolTest, SequencedRunsTasksInCurrentSequence) {
  auto task_runner = test::CreateTaskRunnerWithExecutionMode(
      worker_pool_.get(), GetParam().execution_mode_);
  auto sequenced_task_runner =
      worker_pool_->CreateSequencedTaskRunnerWithTraits(TaskTraits());

  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  task_runner->PostTask(
      FROM_HERE,
      BindOnce(
          [](scoped_refptr<TaskRunner> sequenced_task_runner,
             WaitableEvent* task_ran) {
            EXPECT_FALSE(sequenced_task_runner->RunsTasksInCurrentSequence());
            task_ran->Signal();
          },
          sequenced_task_runner, Unretained(&task_ran)));
  task_ran.Wait();
}

INSTANTIATE_TEST_CASE_P(WinParallel,
                        TaskSchedulerWorkerPoolTest,
                        ::testing::Values(PoolExecutionType{
                            PoolType::WINDOWS, test::ExecutionMode::PARALLEL}));
INSTANTIATE_TEST_CASE_P(WinSequenced,
                        TaskSchedulerWorkerPoolTest,
                        ::testing::Values(PoolExecutionType{
                            PoolType::WINDOWS,
                            test::ExecutionMode::SEQUENCED}));
INSTANTIATE_TEST_CASE_P(GenericSequenced,
                        TaskSchedulerWorkerPoolTest,
                        ::testing::Values(PoolExecutionType{
                            PoolType::GENERIC,
                            test::ExecutionMode::SEQUENCED}));
INSTANTIATE_TEST_CASE_P(GenericParallel,
                        TaskSchedulerWorkerPoolTest,
                        ::testing::Values(PoolExecutionType{
                            PoolType::GENERIC, test::ExecutionMode::PARALLEL}));

}  // namespace internal
}  // namespace base
