// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/task_tracker.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "base/sequence_token.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/atomic_flag.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/scheduler_lock.h"
#include "base/task_scheduler/task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/task_scheduler/test_utils.h"
#include "base/test/gtest_util.h"
#include "base/test/histogram_tester.h"
#include "base/test/test_simple_task_runner.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/simple_thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

namespace {

constexpr size_t kLoadTestNumIterations = 75;

class MockCanScheduleSequenceObserver : public CanScheduleSequenceObserver {
 public:
  void OnCanScheduleSequence(scoped_refptr<Sequence> sequence) override {
    MockOnCanScheduleSequence(sequence.get());
  }

  MOCK_METHOD1(MockOnCanScheduleSequence, void(Sequence*));
};

// Invokes a closure asynchronously.
class CallbackThread : public SimpleThread {
 public:
  explicit CallbackThread(const Closure& closure)
      : SimpleThread("CallbackThread"), closure_(closure) {}

  // Returns true once the callback returns.
  bool has_returned() { return has_returned_.IsSet(); }

 private:
  void Run() override {
    closure_.Run();
    has_returned_.Set();
  }

  const Closure closure_;
  AtomicFlag has_returned_;

  DISALLOW_COPY_AND_ASSIGN(CallbackThread);
};

class ThreadPostingAndRunningTask : public SimpleThread {
 public:
  enum class Action {
    WILL_POST,
    RUN,
    WILL_POST_AND_RUN,
  };

  ThreadPostingAndRunningTask(TaskTracker* tracker,
                              Task* task,
                              Action action,
                              bool expect_post_succeeds)
      : SimpleThread("ThreadPostingAndRunningTask"),
        tracker_(tracker),
        task_(task),
        action_(action),
        expect_post_succeeds_(expect_post_succeeds) {
    EXPECT_TRUE(task_);

    // Ownership of the Task is required to run it.
    EXPECT_NE(Action::RUN, action_);
    EXPECT_NE(Action::WILL_POST_AND_RUN, action_);
  }

  ThreadPostingAndRunningTask(TaskTracker* tracker,
                              std::unique_ptr<Task> task,
                              Action action,
                              bool expect_post_succeeds)
      : SimpleThread("ThreadPostingAndRunningTask"),
        tracker_(tracker),
        task_(task.get()),
        owned_task_(std::move(task)),
        action_(action),
        expect_post_succeeds_(expect_post_succeeds) {
    EXPECT_TRUE(task_);
  }

 private:
  void Run() override {
    bool post_succeeded = true;
    if (action_ == Action::WILL_POST || action_ == Action::WILL_POST_AND_RUN) {
      post_succeeded = tracker_->WillPostTask(task_);
      EXPECT_EQ(expect_post_succeeds_, post_succeeded);
    }
    if (post_succeeded &&
        (action_ == Action::RUN || action_ == Action::WILL_POST_AND_RUN)) {
      EXPECT_TRUE(owned_task_);

      MockCanScheduleSequenceObserver observer;
      auto sequence = tracker_->WillScheduleSequence(
          test::CreateSequenceWithTask(std::move(owned_task_)), &observer);
      ASSERT_TRUE(sequence);
      tracker_->RunNextTask(std::move(sequence), &observer);
    }
  }

  TaskTracker* const tracker_;
  Task* const task_;
  std::unique_ptr<Task> owned_task_;
  const Action action_;
  const bool expect_post_succeeds_;

  DISALLOW_COPY_AND_ASSIGN(ThreadPostingAndRunningTask);
};

class ScopedSetSingletonAllowed {
 public:
  ScopedSetSingletonAllowed(bool singleton_allowed)
      : previous_value_(
            ThreadRestrictions::SetSingletonAllowed(singleton_allowed)) {}
  ~ScopedSetSingletonAllowed() {
    ThreadRestrictions::SetSingletonAllowed(previous_value_);
  }

 private:
  const bool previous_value_;
};

class TaskSchedulerTaskTrackerTest
    : public testing::TestWithParam<TaskShutdownBehavior> {
 protected:
  TaskSchedulerTaskTrackerTest() = default;

  // Creates a task with |shutdown_behavior|.
  std::unique_ptr<Task> CreateTask(TaskShutdownBehavior shutdown_behavior) {
    return std::make_unique<Task>(
        FROM_HERE,
        Bind(&TaskSchedulerTaskTrackerTest::RunTaskCallback, Unretained(this)),
        TaskTraits(shutdown_behavior), TimeDelta());
  }

  void ScheduleAndRunTaskWithTracker(std::unique_ptr<Task> task) {
    auto sequence = tracker_.WillScheduleSequence(
        test::CreateSequenceWithTask(std::move(task)), &observer_);
    ASSERT_TRUE(sequence);
    tracker_.RunNextTask(std::move(sequence), &observer_);
  }

  // Calls tracker_->Shutdown() on a new thread. When this returns, Shutdown()
  // method has been entered on the new thread, but it hasn't necessarily
  // returned.
  void CallShutdownAsync() {
    ASSERT_FALSE(thread_calling_shutdown_);
    thread_calling_shutdown_.reset(new CallbackThread(
        Bind(&TaskTracker::Shutdown, Unretained(&tracker_))));
    thread_calling_shutdown_->Start();
    while (!tracker_.HasShutdownStarted())
      PlatformThread::YieldCurrentThread();
  }

  void WaitForAsyncIsShutdownComplete() {
    ASSERT_TRUE(thread_calling_shutdown_);
    thread_calling_shutdown_->Join();
    EXPECT_TRUE(thread_calling_shutdown_->has_returned());
    EXPECT_TRUE(tracker_.IsShutdownComplete());
  }

  void VerifyAsyncShutdownInProgress() {
    ASSERT_TRUE(thread_calling_shutdown_);
    EXPECT_FALSE(thread_calling_shutdown_->has_returned());
    EXPECT_TRUE(tracker_.HasShutdownStarted());
    EXPECT_FALSE(tracker_.IsShutdownComplete());
  }

  // Calls tracker_->Flush() on a new thread.
  void CallFlushAsync() {
    ASSERT_FALSE(thread_calling_flush_);
    thread_calling_flush_.reset(
        new CallbackThread(Bind(&TaskTracker::Flush, Unretained(&tracker_))));
    thread_calling_flush_->Start();
  }

  void WaitForAsyncFlushReturned() {
    ASSERT_TRUE(thread_calling_flush_);
    thread_calling_flush_->Join();
    EXPECT_TRUE(thread_calling_flush_->has_returned());
  }

  void VerifyAsyncFlushInProgress() {
    ASSERT_TRUE(thread_calling_flush_);
    EXPECT_FALSE(thread_calling_flush_->has_returned());
  }

  size_t NumTasksExecuted() {
    AutoSchedulerLock auto_lock(lock_);
    return num_tasks_executed_;
  }

  TaskTracker tracker_;
  MockCanScheduleSequenceObserver observer_;

 private:
  void RunTaskCallback() {
    AutoSchedulerLock auto_lock(lock_);
    ++num_tasks_executed_;
  }

  std::unique_ptr<CallbackThread> thread_calling_shutdown_;
  std::unique_ptr<CallbackThread> thread_calling_flush_;

  // Synchronizes accesses to |num_tasks_executed_|.
  SchedulerLock lock_;

  size_t num_tasks_executed_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerTaskTrackerTest);
};

#define WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED() \
  do {                                      \
    SCOPED_TRACE("");                       \
    WaitForAsyncIsShutdownComplete();       \
  } while (false)

#define VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS() \
  do {                                      \
    SCOPED_TRACE("");                       \
    VerifyAsyncShutdownInProgress();        \
  } while (false)

#define WAIT_FOR_ASYNC_FLUSH_RETURNED() \
  do {                                  \
    SCOPED_TRACE("");                   \
    WaitForAsyncFlushReturned();        \
  } while (false)

#define VERIFY_ASYNC_FLUSH_IN_PROGRESS() \
  do {                                   \
    SCOPED_TRACE("");                    \
    VerifyAsyncFlushInProgress();        \
  } while (false)

}  // namespace

TEST_P(TaskSchedulerTaskTrackerTest, WillPostAndRunBeforeShutdown) {
  std::unique_ptr<Task> task(CreateTask(GetParam()));

  // Inform |task_tracker_| that |task| will be posted.
  EXPECT_TRUE(tracker_.WillPostTask(task.get()));

  // Run the task.
  EXPECT_EQ(0U, NumTasksExecuted());

  ScheduleAndRunTaskWithTracker(std::move(task));
  EXPECT_EQ(1U, NumTasksExecuted());

  // Shutdown() shouldn't block.
  tracker_.Shutdown();
}

TEST_P(TaskSchedulerTaskTrackerTest, WillPostAndRunLongTaskBeforeShutdown) {
  // Create a task that signals |task_running| and blocks until |task_barrier|
  // is signaled.
  WaitableEvent task_running(WaitableEvent::ResetPolicy::AUTOMATIC,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  WaitableEvent task_barrier(WaitableEvent::ResetPolicy::AUTOMATIC,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  auto blocked_task = std::make_unique<Task>(
      FROM_HERE,
      Bind(
          [](WaitableEvent* task_running, WaitableEvent* task_barrier) {
            task_running->Signal();
            task_barrier->Wait();
          },
          Unretained(&task_running), Unretained(&task_barrier)),
      TaskTraits(WithBaseSyncPrimitives(), GetParam()), TimeDelta());

  // Inform |task_tracker_| that |blocked_task| will be posted.
  EXPECT_TRUE(tracker_.WillPostTask(blocked_task.get()));

  // Create a thread to run the task. Wait until the task starts running.
  ThreadPostingAndRunningTask thread_running_task(
      &tracker_, std::move(blocked_task),
      ThreadPostingAndRunningTask::Action::RUN, false);
  thread_running_task.Start();
  task_running.Wait();

  // Initiate shutdown after the task has been scheduled.
  CallShutdownAsync();

  if (GetParam() == TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN) {
    // Shutdown should complete even with a CONTINUE_ON_SHUTDOWN in progress.
    WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();
  } else {
    // Shutdown should block with any non CONTINUE_ON_SHUTDOWN task in progress.
    VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();
  }

  // Unblock the task.
  task_barrier.Signal();
  thread_running_task.Join();

  // Shutdown should now complete for a non CONTINUE_ON_SHUTDOWN task.
  if (GetParam() != TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN)
    WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();
}

TEST_P(TaskSchedulerTaskTrackerTest, WillPostBeforeShutdownRunDuringShutdown) {
  // Inform |task_tracker_| that a task will be posted.
  std::unique_ptr<Task> task(CreateTask(GetParam()));
  EXPECT_TRUE(tracker_.WillPostTask(task.get()));

  // Inform |task_tracker_| that a BLOCK_SHUTDOWN task will be posted just to
  // block shutdown.
  std::unique_ptr<Task> block_shutdown_task(
      CreateTask(TaskShutdownBehavior::BLOCK_SHUTDOWN));
  EXPECT_TRUE(tracker_.WillPostTask(block_shutdown_task.get()));

  // Call Shutdown() asynchronously.
  CallShutdownAsync();
  VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();

  // Try to run |task|. It should only run it it's BLOCK_SHUTDOWN. Otherwise it
  // should be discarded.
  EXPECT_EQ(0U, NumTasksExecuted());
  const bool should_run = GetParam() == TaskShutdownBehavior::BLOCK_SHUTDOWN;

  ScheduleAndRunTaskWithTracker(std::move(task));
  EXPECT_EQ(should_run ? 1U : 0U, NumTasksExecuted());
  VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();

  // Unblock shutdown by running the remaining BLOCK_SHUTDOWN task.
  ScheduleAndRunTaskWithTracker(std::move(block_shutdown_task));
  EXPECT_EQ(should_run ? 2U : 1U, NumTasksExecuted());
  WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();
}

TEST_P(TaskSchedulerTaskTrackerTest, WillPostBeforeShutdownRunAfterShutdown) {
  // Inform |task_tracker_| that a task will be posted.
  std::unique_ptr<Task> task(CreateTask(GetParam()));
  EXPECT_TRUE(tracker_.WillPostTask(task.get()));

  // Call Shutdown() asynchronously.
  CallShutdownAsync();
  EXPECT_EQ(0U, NumTasksExecuted());

  if (GetParam() == TaskShutdownBehavior::BLOCK_SHUTDOWN) {
    VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();

    // Run the task to unblock shutdown.
    ScheduleAndRunTaskWithTracker(std::move(task));
    EXPECT_EQ(1U, NumTasksExecuted());
    WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();

    // It is not possible to test running a BLOCK_SHUTDOWN task posted before
    // shutdown after shutdown because Shutdown() won't return if there are
    // pending BLOCK_SHUTDOWN tasks.
  } else {
    WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();

    // The task shouldn't be allowed to run after shutdown.
    ScheduleAndRunTaskWithTracker(std::move(task));
    EXPECT_EQ(0U, NumTasksExecuted());
  }
}

TEST_P(TaskSchedulerTaskTrackerTest, WillPostAndRunDuringShutdown) {
  // Inform |task_tracker_| that a BLOCK_SHUTDOWN task will be posted just to
  // block shutdown.
  std::unique_ptr<Task> block_shutdown_task(
      CreateTask(TaskShutdownBehavior::BLOCK_SHUTDOWN));
  EXPECT_TRUE(tracker_.WillPostTask(block_shutdown_task.get()));

  // Call Shutdown() asynchronously.
  CallShutdownAsync();
  VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();

  if (GetParam() == TaskShutdownBehavior::BLOCK_SHUTDOWN) {
    // Inform |task_tracker_| that a BLOCK_SHUTDOWN task will be posted.
    std::unique_ptr<Task> task(CreateTask(GetParam()));
    EXPECT_TRUE(tracker_.WillPostTask(task.get()));

    // Run the BLOCK_SHUTDOWN task.
    EXPECT_EQ(0U, NumTasksExecuted());
    ScheduleAndRunTaskWithTracker(std::move(task));
    EXPECT_EQ(1U, NumTasksExecuted());
  } else {
    // It shouldn't be allowed to post a non BLOCK_SHUTDOWN task.
    std::unique_ptr<Task> task(CreateTask(GetParam()));
    EXPECT_FALSE(tracker_.WillPostTask(task.get()));

    // Don't try to run the task, because it wasn't allowed to be posted.
  }

  // Unblock shutdown by running |block_shutdown_task|.
  VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();
  ScheduleAndRunTaskWithTracker(std::move(block_shutdown_task));
  EXPECT_EQ(GetParam() == TaskShutdownBehavior::BLOCK_SHUTDOWN ? 2U : 1U,
            NumTasksExecuted());
  WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();
}

TEST_P(TaskSchedulerTaskTrackerTest, WillPostAfterShutdown) {
  tracker_.Shutdown();

  std::unique_ptr<Task> task(CreateTask(GetParam()));

  // |task_tracker_| shouldn't allow a task to be posted after shutdown.
  EXPECT_FALSE(tracker_.WillPostTask(task.get()));
}

// Verify that BLOCK_SHUTDOWN and SKIP_ON_SHUTDOWN tasks can
// AssertSingletonAllowed() but CONTINUE_ON_SHUTDOWN tasks can't.
TEST_P(TaskSchedulerTaskTrackerTest, SingletonAllowed) {
  const bool can_use_singletons =
      (GetParam() != TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN);

  std::unique_ptr<Task> task(
      new Task(FROM_HERE, BindOnce(&ThreadRestrictions::AssertSingletonAllowed),
               TaskTraits(GetParam()), TimeDelta()));
  EXPECT_TRUE(tracker_.WillPostTask(task.get()));

  // Set the singleton allowed bit to the opposite of what it is expected to be
  // when |tracker| runs |task| to verify that |tracker| actually sets the
  // correct value.
  ScopedSetSingletonAllowed scoped_singleton_allowed(!can_use_singletons);

  // Running the task should fail iff the task isn't allowed to use singletons.
  if (can_use_singletons) {
    ScheduleAndRunTaskWithTracker(std::move(task));
  } else {
    EXPECT_DCHECK_DEATH({ ScheduleAndRunTaskWithTracker(std::move(task)); });
  }
}

// Verify that AssertIOAllowed() succeeds only for a MayBlock() task.
TEST_P(TaskSchedulerTaskTrackerTest, IOAllowed) {
  // Unset the IO allowed bit. Expect TaskTracker to set it before running a
  // task with the MayBlock() trait.
  ThreadRestrictions::SetIOAllowed(false);
  auto task_with_may_block =
      std::make_unique<Task>(FROM_HERE, Bind([]() {
                               // Shouldn't fail.
                               ThreadRestrictions::AssertIOAllowed();
                             }),
                             TaskTraits(MayBlock(), GetParam()), TimeDelta());
  EXPECT_TRUE(tracker_.WillPostTask(task_with_may_block.get()));
  ScheduleAndRunTaskWithTracker(std::move(task_with_may_block));

  // Set the IO allowed bit. Expect TaskTracker to unset it before running a
  // task without the MayBlock() trait.
  ThreadRestrictions::SetIOAllowed(true);
  auto task_without_may_block = std::make_unique<Task>(
      FROM_HERE, Bind([]() {
        EXPECT_DCHECK_DEATH({ ThreadRestrictions::AssertIOAllowed(); });
      }),
      TaskTraits(GetParam()), TimeDelta());
  EXPECT_TRUE(tracker_.WillPostTask(task_without_may_block.get()));
  ScheduleAndRunTaskWithTracker(std::move(task_without_may_block));
}

static void RunTaskRunnerHandleVerificationTask(
    TaskTracker* tracker,
    std::unique_ptr<Task> verify_task) {
  // Pretend |verify_task| is posted to respect TaskTracker's contract.
  EXPECT_TRUE(tracker->WillPostTask(verify_task.get()));

  // Confirm that the test conditions are right (no TaskRunnerHandles set
  // already).
  EXPECT_FALSE(ThreadTaskRunnerHandle::IsSet());
  EXPECT_FALSE(SequencedTaskRunnerHandle::IsSet());

  MockCanScheduleSequenceObserver observer;
  auto sequence = tracker->WillScheduleSequence(
      test::CreateSequenceWithTask(std::move(verify_task)), &observer);
  ASSERT_TRUE(sequence);
  tracker->RunNextTask(std::move(sequence), &observer);

  // TaskRunnerHandle state is reset outside of task's scope.
  EXPECT_FALSE(ThreadTaskRunnerHandle::IsSet());
  EXPECT_FALSE(SequencedTaskRunnerHandle::IsSet());
}

static void VerifyNoTaskRunnerHandle() {
  EXPECT_FALSE(ThreadTaskRunnerHandle::IsSet());
  EXPECT_FALSE(SequencedTaskRunnerHandle::IsSet());
}

TEST_P(TaskSchedulerTaskTrackerTest, TaskRunnerHandleIsNotSetOnParallel) {
  // Create a task that will verify that TaskRunnerHandles are not set in its
  // scope per no TaskRunner ref being set to it.
  std::unique_ptr<Task> verify_task(
      new Task(FROM_HERE, BindOnce(&VerifyNoTaskRunnerHandle),
               TaskTraits(GetParam()), TimeDelta()));

  RunTaskRunnerHandleVerificationTask(&tracker_, std::move(verify_task));
}

static void VerifySequencedTaskRunnerHandle(
    const SequencedTaskRunner* expected_task_runner) {
  EXPECT_FALSE(ThreadTaskRunnerHandle::IsSet());
  EXPECT_TRUE(SequencedTaskRunnerHandle::IsSet());
  EXPECT_EQ(expected_task_runner, SequencedTaskRunnerHandle::Get());
}

TEST_P(TaskSchedulerTaskTrackerTest,
       SequencedTaskRunnerHandleIsSetOnSequenced) {
  scoped_refptr<SequencedTaskRunner> test_task_runner(new TestSimpleTaskRunner);

  // Create a task that will verify that SequencedTaskRunnerHandle is properly
  // set to |test_task_runner| in its scope per |sequenced_task_runner_ref|
  // being set to it.
  std::unique_ptr<Task> verify_task(
      new Task(FROM_HERE,
               BindOnce(&VerifySequencedTaskRunnerHandle,
                        Unretained(test_task_runner.get())),
               TaskTraits(GetParam()), TimeDelta()));
  verify_task->sequenced_task_runner_ref = test_task_runner;

  RunTaskRunnerHandleVerificationTask(&tracker_, std::move(verify_task));
}

static void VerifyThreadTaskRunnerHandle(
    const SingleThreadTaskRunner* expected_task_runner) {
  EXPECT_TRUE(ThreadTaskRunnerHandle::IsSet());
  // SequencedTaskRunnerHandle inherits ThreadTaskRunnerHandle for thread.
  EXPECT_TRUE(SequencedTaskRunnerHandle::IsSet());
  EXPECT_EQ(expected_task_runner, ThreadTaskRunnerHandle::Get());
}

TEST_P(TaskSchedulerTaskTrackerTest,
       ThreadTaskRunnerHandleIsSetOnSingleThreaded) {
  scoped_refptr<SingleThreadTaskRunner> test_task_runner(
      new TestSimpleTaskRunner);

  // Create a task that will verify that ThreadTaskRunnerHandle is properly set
  // to |test_task_runner| in its scope per |single_thread_task_runner_ref|
  // being set on it.
  std::unique_ptr<Task> verify_task(
      new Task(FROM_HERE,
               BindOnce(&VerifyThreadTaskRunnerHandle,
                        Unretained(test_task_runner.get())),
               TaskTraits(GetParam()), TimeDelta()));
  verify_task->single_thread_task_runner_ref = test_task_runner;

  RunTaskRunnerHandleVerificationTask(&tracker_, std::move(verify_task));
}

TEST_P(TaskSchedulerTaskTrackerTest, FlushPendingDelayedTask) {
  const Task delayed_task(FROM_HERE, BindOnce(&DoNothing),
                          TaskTraits(GetParam()), TimeDelta::FromDays(1));
  tracker_.WillPostTask(&delayed_task);
  // Flush() should return even if the delayed task didn't run.
  tracker_.Flush();
}

TEST_P(TaskSchedulerTaskTrackerTest, FlushPendingUndelayedTask) {
  auto undelayed_task = std::make_unique<Task>(
      FROM_HERE, Bind(&DoNothing), TaskTraits(GetParam()), TimeDelta());
  tracker_.WillPostTask(undelayed_task.get());

  // Flush() shouldn't return before the undelayed task runs.
  CallFlushAsync();
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  VERIFY_ASYNC_FLUSH_IN_PROGRESS();

  // Flush() should return after the undelayed task runs.
  ScheduleAndRunTaskWithTracker(std::move(undelayed_task));
  WAIT_FOR_ASYNC_FLUSH_RETURNED();
}

TEST_P(TaskSchedulerTaskTrackerTest, PostTaskDuringFlush) {
  auto undelayed_task = std::make_unique<Task>(
      FROM_HERE, Bind(&DoNothing), TaskTraits(GetParam()), TimeDelta());
  tracker_.WillPostTask(undelayed_task.get());

  // Flush() shouldn't return before the undelayed task runs.
  CallFlushAsync();
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  VERIFY_ASYNC_FLUSH_IN_PROGRESS();

  // Simulate posting another undelayed task.
  auto other_undelayed_task = std::make_unique<Task>(
      FROM_HERE, Bind(&DoNothing), TaskTraits(GetParam()), TimeDelta());
  tracker_.WillPostTask(other_undelayed_task.get());

  // Run the first undelayed task.
  ScheduleAndRunTaskWithTracker(std::move(undelayed_task));

  // Flush() shouldn't return before the second undelayed task runs.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  VERIFY_ASYNC_FLUSH_IN_PROGRESS();

  // Flush() should return after the second undelayed task runs.
  ScheduleAndRunTaskWithTracker(std::move(other_undelayed_task));
  WAIT_FOR_ASYNC_FLUSH_RETURNED();
}

TEST_P(TaskSchedulerTaskTrackerTest, RunDelayedTaskDuringFlush) {
  // Simulate posting a delayed and an undelayed task.
  auto delayed_task =
      std::make_unique<Task>(FROM_HERE, Bind(&DoNothing),
                             TaskTraits(GetParam()), TimeDelta::FromDays(1));
  tracker_.WillPostTask(delayed_task.get());
  auto undelayed_task = std::make_unique<Task>(
      FROM_HERE, Bind(&DoNothing), TaskTraits(GetParam()), TimeDelta());
  tracker_.WillPostTask(undelayed_task.get());

  // Flush() shouldn't return before the undelayed task runs.
  CallFlushAsync();
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  VERIFY_ASYNC_FLUSH_IN_PROGRESS();

  // Run the delayed task.
  ScheduleAndRunTaskWithTracker(std::move(delayed_task));

  // Flush() shouldn't return since there is still a pending undelayed
  // task.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  VERIFY_ASYNC_FLUSH_IN_PROGRESS();

  // Run the undelayed task.
  ScheduleAndRunTaskWithTracker(std::move(undelayed_task));

  // Flush() should now return.
  WAIT_FOR_ASYNC_FLUSH_RETURNED();
}

TEST_P(TaskSchedulerTaskTrackerTest, FlushAfterShutdown) {
  if (GetParam() == TaskShutdownBehavior::BLOCK_SHUTDOWN)
    return;

  // Simulate posting a task.
  auto undelayed_task = std::make_unique<Task>(
      FROM_HERE, Bind(&DoNothing), TaskTraits(GetParam()), TimeDelta());
  tracker_.WillPostTask(undelayed_task.get());

  // Shutdown() should return immediately since there are no pending
  // BLOCK_SHUTDOWN tasks.
  tracker_.Shutdown();

  // Flush() should return immediately after shutdown, even if an
  // undelayed task hasn't run.
  tracker_.Flush();
}

TEST_P(TaskSchedulerTaskTrackerTest, ShutdownDuringFlush) {
  if (GetParam() == TaskShutdownBehavior::BLOCK_SHUTDOWN)
    return;

  // Simulate posting a task.
  auto undelayed_task = std::make_unique<Task>(
      FROM_HERE, Bind(&DoNothing), TaskTraits(GetParam()), TimeDelta());
  tracker_.WillPostTask(undelayed_task.get());

  // Flush() shouldn't return before the undelayed task runs or
  // shutdown completes.
  CallFlushAsync();
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  VERIFY_ASYNC_FLUSH_IN_PROGRESS();

  // Shutdown() should return immediately since there are no pending
  // BLOCK_SHUTDOWN tasks.
  tracker_.Shutdown();

  // Flush() should now return, even if an undelayed task hasn't run.
  WAIT_FOR_ASYNC_FLUSH_RETURNED();
}

INSTANTIATE_TEST_CASE_P(
    ContinueOnShutdown,
    TaskSchedulerTaskTrackerTest,
    ::testing::Values(TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN));
INSTANTIATE_TEST_CASE_P(
    SkipOnShutdown,
    TaskSchedulerTaskTrackerTest,
    ::testing::Values(TaskShutdownBehavior::SKIP_ON_SHUTDOWN));
INSTANTIATE_TEST_CASE_P(
    BlockShutdown,
    TaskSchedulerTaskTrackerTest,
    ::testing::Values(TaskShutdownBehavior::BLOCK_SHUTDOWN));

namespace {

void ExpectSequenceToken(SequenceToken sequence_token) {
  EXPECT_EQ(sequence_token, SequenceToken::GetForCurrentThread());
}

}  // namespace

// Verify that SequenceToken::GetForCurrentThread() returns the Sequence's token
// when a Task runs.
TEST_F(TaskSchedulerTaskTrackerTest, CurrentSequenceToken) {
  scoped_refptr<Sequence> sequence = MakeRefCounted<Sequence>();

  const SequenceToken sequence_token = sequence->token();
  auto task = std::make_unique<Task>(FROM_HERE,
                                     Bind(&ExpectSequenceToken, sequence_token),
                                     TaskTraits(), TimeDelta());
  tracker_.WillPostTask(task.get());

  sequence->PushTask(std::move(task));

  EXPECT_FALSE(SequenceToken::GetForCurrentThread().IsValid());
  sequence = tracker_.WillScheduleSequence(std::move(sequence), &observer_);
  ASSERT_TRUE(sequence);
  tracker_.RunNextTask(std::move(sequence), &observer_);
  EXPECT_FALSE(SequenceToken::GetForCurrentThread().IsValid());
}

TEST_F(TaskSchedulerTaskTrackerTest, LoadWillPostAndRunBeforeShutdown) {
  // Post and run tasks asynchronously.
  std::vector<std::unique_ptr<ThreadPostingAndRunningTask>> threads;

  for (size_t i = 0; i < kLoadTestNumIterations; ++i) {
    threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, CreateTask(TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN),
        ThreadPostingAndRunningTask::Action::WILL_POST_AND_RUN, true));
    threads.back()->Start();

    threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, CreateTask(TaskShutdownBehavior::SKIP_ON_SHUTDOWN),
        ThreadPostingAndRunningTask::Action::WILL_POST_AND_RUN, true));
    threads.back()->Start();

    threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, CreateTask(TaskShutdownBehavior::BLOCK_SHUTDOWN),
        ThreadPostingAndRunningTask::Action::WILL_POST_AND_RUN, true));
    threads.back()->Start();
  }

  for (const auto& thread : threads)
    thread->Join();

  // Expect all tasks to be executed.
  EXPECT_EQ(kLoadTestNumIterations * 3, NumTasksExecuted());

  // Should return immediately because no tasks are blocking shutdown.
  tracker_.Shutdown();
}

TEST_F(TaskSchedulerTaskTrackerTest,
       LoadWillPostBeforeShutdownAndRunDuringShutdown) {
  // Post tasks asynchronously.
  std::vector<std::unique_ptr<Task>> tasks;
  std::vector<std::unique_ptr<ThreadPostingAndRunningTask>> post_threads;

  for (size_t i = 0; i < kLoadTestNumIterations; ++i) {
    tasks.push_back(CreateTask(TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN));
    post_threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, tasks.back().get(),
        ThreadPostingAndRunningTask::Action::WILL_POST, true));
    post_threads.back()->Start();

    tasks.push_back(CreateTask(TaskShutdownBehavior::SKIP_ON_SHUTDOWN));
    post_threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, tasks.back().get(),
        ThreadPostingAndRunningTask::Action::WILL_POST, true));
    post_threads.back()->Start();

    tasks.push_back(CreateTask(TaskShutdownBehavior::BLOCK_SHUTDOWN));
    post_threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, tasks.back().get(),
        ThreadPostingAndRunningTask::Action::WILL_POST, true));
    post_threads.back()->Start();
  }

  for (const auto& thread : post_threads)
    thread->Join();

  // Call Shutdown() asynchronously.
  CallShutdownAsync();

  // Run tasks asynchronously.
  std::vector<std::unique_ptr<ThreadPostingAndRunningTask>> run_threads;

  for (auto& task : tasks) {
    run_threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, std::move(task), ThreadPostingAndRunningTask::Action::RUN,
        false));
    run_threads.back()->Start();
  }

  for (const auto& thread : run_threads)
    thread->Join();

  WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();

  // Expect BLOCK_SHUTDOWN tasks to have been executed.
  EXPECT_EQ(kLoadTestNumIterations, NumTasksExecuted());
}

TEST_F(TaskSchedulerTaskTrackerTest, LoadWillPostAndRunDuringShutdown) {
  // Inform |task_tracker_| that a BLOCK_SHUTDOWN task will be posted just to
  // block shutdown.
  std::unique_ptr<Task> block_shutdown_task(
      CreateTask(TaskShutdownBehavior::BLOCK_SHUTDOWN));
  EXPECT_TRUE(tracker_.WillPostTask(block_shutdown_task.get()));

  // Call Shutdown() asynchronously.
  CallShutdownAsync();

  // Post and run tasks asynchronously.
  std::vector<std::unique_ptr<ThreadPostingAndRunningTask>> threads;

  for (size_t i = 0; i < kLoadTestNumIterations; ++i) {
    threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, CreateTask(TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN),
        ThreadPostingAndRunningTask::Action::WILL_POST_AND_RUN, false));
    threads.back()->Start();

    threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, CreateTask(TaskShutdownBehavior::SKIP_ON_SHUTDOWN),
        ThreadPostingAndRunningTask::Action::WILL_POST_AND_RUN, false));
    threads.back()->Start();

    threads.push_back(std::make_unique<ThreadPostingAndRunningTask>(
        &tracker_, CreateTask(TaskShutdownBehavior::BLOCK_SHUTDOWN),
        ThreadPostingAndRunningTask::Action::WILL_POST_AND_RUN, true));
    threads.back()->Start();
  }

  for (const auto& thread : threads)
    thread->Join();

  // Expect BLOCK_SHUTDOWN tasks to have been executed.
  EXPECT_EQ(kLoadTestNumIterations, NumTasksExecuted());

  // Shutdown() shouldn't return before |block_shutdown_task| is executed.
  VERIFY_ASYNC_SHUTDOWN_IN_PROGRESS();

  // Unblock shutdown by running |block_shutdown_task|.
  ScheduleAndRunTaskWithTracker(std::move(block_shutdown_task));
  EXPECT_EQ(kLoadTestNumIterations + 1, NumTasksExecuted());
  WAIT_FOR_ASYNC_SHUTDOWN_COMPLETED();
}

// Verify that WillScheduleSequence() returns nullptr when it receives a
// background sequence and there is pending foreground work. Verify that the
// observer is notified when there is no more pending foreground work.
TEST_F(TaskSchedulerTaskTrackerTest,
       WillScheduleBackgroundSequenceWithPendingForegroundWork) {
  // Simulate posting a foreground task.
  bool foreground_task_did_run = false;
  auto foreground_task =
      MakeUnique<Task>(FROM_HERE,
                       BindOnce(
                           [](bool* foreground_task_did_run) {
                             *foreground_task_did_run = true;
                           },
                           Unretained(&foreground_task_did_run)),
                       TaskTraits(TaskPriority::USER_VISIBLE), TimeDelta());
  EXPECT_TRUE(tracker_.WillPostTask(foreground_task.get()));
  testing::StrictMock<MockCanScheduleSequenceObserver> foreground_observer;
  scoped_refptr<Sequence> foreground_sequence =
      test::CreateSequenceWithTask(std::move(foreground_task));
  EXPECT_EQ(
      foreground_sequence,
      tracker_.WillScheduleSequence(foreground_sequence, &foreground_observer));

  // Simulate posting a background task. This should fail because there is a
  // pending foreground task.
  bool background_task_did_run = false;
  auto background_task =
      MakeUnique<Task>(FROM_HERE,
                       BindOnce(
                           [](bool* background_task_did_run) {
                             *background_task_did_run = true;
                           },
                           Unretained(&background_task_did_run)),
                       TaskTraits(TaskPriority::BACKGROUND), TimeDelta());
  EXPECT_TRUE(tracker_.WillPostTask(background_task.get()));
  testing::StrictMock<MockCanScheduleSequenceObserver> background_observer;
  scoped_refptr<Sequence> background_sequence =
      test::CreateSequenceWithTask(std::move(background_task));
  EXPECT_FALSE(
      tracker_.WillScheduleSequence(background_sequence, &background_observer));

  // Run the foreground sequence. Expect |background_observer| to be notified
  // that |background_sequence| can be scheduled.
  EXPECT_CALL(background_observer,
              MockOnCanScheduleSequence(background_sequence.get()));
  EXPECT_FALSE(foreground_task_did_run);
  EXPECT_FALSE(tracker_.RunNextTask(std::move(foreground_sequence),
                                    &foreground_observer));
  testing::Mock::VerifyAndClear(&background_observer);
  EXPECT_TRUE(foreground_task_did_run);

  // Run |background_sequence|. This should succeed.
  EXPECT_FALSE(background_task_did_run);
  EXPECT_FALSE(tracker_.RunNextTask(background_sequence, &background_observer));
  EXPECT_TRUE(background_task_did_run);
}

// Verify that WillScheduleSequence() returns nullptr when it receives a
// background sequence and the maximum number of background sequences that can
// be scheduled concurrently is reached. Verify that an observer is notified
// when a background sequence can be scheduled (i.e. when one of the previously
// scheduled background sequences has run).
TEST_F(TaskSchedulerTaskTrackerTest,
       WillScheduleBackgroundSequenceWithMaxBackgroundSequences) {
  constexpr int kMaxNumScheduledBackgroundSequences = 2;
  TaskTracker tracker(kMaxNumScheduledBackgroundSequences);

  // Simulate posting and scheduling |kMaxNumScheduledBackgroundSequences|
  // background sequences. This should succeed.
  std::vector<scoped_refptr<Sequence>> scheduled_sequences;
  testing::StrictMock<MockCanScheduleSequenceObserver> never_notified_observer;
  for (int i = 0; i < kMaxNumScheduledBackgroundSequences; ++i) {
    auto task =
        MakeUnique<Task>(FROM_HERE, BindOnce(&DoNothing),
                         TaskTraits(TaskPriority::BACKGROUND), TimeDelta());
    EXPECT_TRUE(tracker.WillPostTask(task.get()));
    scoped_refptr<Sequence> sequence =
        test::CreateSequenceWithTask(std::move(task));
    EXPECT_EQ(sequence,
              tracker.WillScheduleSequence(sequence, &never_notified_observer));
    scheduled_sequences.push_back(std::move(sequence));
  }

  // Simulate posting and scheduling extra background sequences. This should
  // fail because the maximum number of background sequences that can be
  // scheduled concurrently is already reached.
  std::vector<std::unique_ptr<bool>> extra_tasks_did_run;
  std::vector<std::unique_ptr<MockCanScheduleSequenceObserver>> extra_observers;
  std::vector<scoped_refptr<Sequence>> extra_sequences;
  for (int i = 0; i < kMaxNumScheduledBackgroundSequences; ++i) {
    extra_tasks_did_run.push_back(MakeUnique<bool>());
    auto extra_task = MakeUnique<Task>(
        FROM_HERE,
        BindOnce([](bool* extra_task_did_run) { *extra_task_did_run = true; },
                 Unretained(extra_tasks_did_run.back().get())),
        TaskTraits(TaskPriority::BACKGROUND), TimeDelta());
    EXPECT_TRUE(tracker.WillPostTask(extra_task.get()));
    extra_sequences.push_back(
        test::CreateSequenceWithTask(std::move(extra_task)));
    extra_observers.push_back(MakeUnique<MockCanScheduleSequenceObserver>());
    EXPECT_EQ(nullptr,
              tracker.WillScheduleSequence(extra_sequences.back(),
                                           extra_observers.back().get()));
  }

  // Run the sequences scheduled at the beginning of the test. Expect an
  // observer from |extra_observer| to be notified every time a task finishes to
  // run.
  for (int i = 0; i < kMaxNumScheduledBackgroundSequences; ++i) {
    EXPECT_CALL(*extra_observers[i].get(),
                MockOnCanScheduleSequence(extra_sequences[i].get()));
    EXPECT_FALSE(
        tracker.RunNextTask(scheduled_sequences[i], &never_notified_observer));
    testing::Mock::VerifyAndClear(extra_observers[i].get());
  }

  // Run the extra sequences.
  for (int i = 0; i < kMaxNumScheduledBackgroundSequences; ++i) {
    EXPECT_FALSE(*extra_tasks_did_run[i]);
    EXPECT_FALSE(
        tracker.RunNextTask(extra_sequences[i], &never_notified_observer));
    EXPECT_TRUE(*extra_tasks_did_run[i]);
  }
}

// Verify that RunNextTask() doesn't run a background task if there is pending
// foreground work. Verify that the observer is notified when there is no more
// pending foreground work.
TEST_F(TaskSchedulerTaskTrackerTest,
       RunNextBackgroundTaskWithPendingForegroundWork) {
  // Simulate posting a background task. This should succeed.
  bool background_task_did_run = false;
  auto background_task =
      MakeUnique<Task>(FROM_HERE,
                       BindOnce(
                           [](bool* background_task_did_run) {
                             *background_task_did_run = true;
                           },
                           Unretained(&background_task_did_run)),
                       TaskTraits(TaskPriority::BACKGROUND), TimeDelta());
  EXPECT_TRUE(tracker_.WillPostTask(background_task.get()));
  testing::StrictMock<MockCanScheduleSequenceObserver> background_observer;
  scoped_refptr<Sequence> background_sequence =
      test::CreateSequenceWithTask(std::move(background_task));
  EXPECT_EQ(
      background_sequence,
      tracker_.WillScheduleSequence(background_sequence, &background_observer));

  // Simulate posting a foreground task.
  bool foreground_task_did_run = false;
  auto foreground_task =
      MakeUnique<Task>(FROM_HERE,
                       BindOnce(
                           [](bool* foreground_task_did_run) {
                             *foreground_task_did_run = true;
                           },
                           Unretained(&foreground_task_did_run)),
                       TaskTraits(TaskPriority::USER_VISIBLE), TimeDelta());
  EXPECT_TRUE(tracker_.WillPostTask(foreground_task.get()));
  testing::StrictMock<MockCanScheduleSequenceObserver> foreground_observer;
  scoped_refptr<Sequence> foreground_sequence =
      test::CreateSequenceWithTask(std::move(foreground_task));
  EXPECT_EQ(
      foreground_sequence,
      tracker_.WillScheduleSequence(foreground_sequence, &foreground_observer));

  // Try to run the background task. This should fail because there is a pending
  // foreground task.
  EXPECT_FALSE(tracker_.RunNextTask(background_sequence, &background_observer));
  EXPECT_FALSE(background_task_did_run);

  // Run the foreground task. Expect |background_observer| to be notified that
  // |background_sequence| can be scheduled.
  EXPECT_FALSE(foreground_task_did_run);
  EXPECT_CALL(background_observer,
              MockOnCanScheduleSequence(background_sequence.get()));
  EXPECT_FALSE(tracker_.RunNextTask(foreground_sequence, &foreground_observer));
  testing::Mock::VerifyAndClear(&background_observer);
  EXPECT_TRUE(foreground_task_did_run);

  // Run the background sequence. This should succeed.
  EXPECT_FALSE(background_task_did_run);
  EXPECT_FALSE(tracker_.RunNextTask(background_sequence, &background_observer));
  EXPECT_TRUE(background_task_did_run);
}

namespace {

class WaitAllowedTestThread : public SimpleThread {
 public:
  WaitAllowedTestThread() : SimpleThread("WaitAllowedTestThread") {}

 private:
  void Run() override {
    TaskTracker tracker;

    // Waiting is allowed by default. Expect TaskTracker to disallow it before
    // running a task without the WithBaseSyncPrimitives() trait.
    ThreadRestrictions::AssertWaitAllowed();
    auto task_without_sync_primitives = std::make_unique<Task>(
        FROM_HERE, Bind([]() {
          EXPECT_DCHECK_DEATH({ ThreadRestrictions::AssertWaitAllowed(); });
        }),
        TaskTraits(), TimeDelta());
    EXPECT_TRUE(tracker.WillPostTask(task_without_sync_primitives.get()));
    MockCanScheduleSequenceObserver observer;
    auto sequence_without_sync_primitives = tracker.WillScheduleSequence(
        test::CreateSequenceWithTask(std::move(task_without_sync_primitives)),
        &observer);
    ASSERT_TRUE(sequence_without_sync_primitives);
    tracker.RunNextTask(std::move(sequence_without_sync_primitives), &observer);

    // Disallow waiting. Expect TaskTracker to allow it before running a task
    // with the WithBaseSyncPrimitives() trait.
    ThreadRestrictions::DisallowWaiting();
    auto task_with_sync_primitives = std::make_unique<Task>(
        FROM_HERE, Bind([]() {
          // Shouldn't fail.
          ThreadRestrictions::AssertWaitAllowed();
        }),
        TaskTraits(WithBaseSyncPrimitives()), TimeDelta());
    EXPECT_TRUE(tracker.WillPostTask(task_with_sync_primitives.get()));
    auto sequence_with_sync_primitives = tracker.WillScheduleSequence(
        test::CreateSequenceWithTask(std::move(task_with_sync_primitives)),
        &observer);
    ASSERT_TRUE(sequence_with_sync_primitives);
    tracker.RunNextTask(std::move(sequence_with_sync_primitives), &observer);
  }

  DISALLOW_COPY_AND_ASSIGN(WaitAllowedTestThread);
};

}  // namespace

// Verify that AssertIOAllowed() succeeds only for a WithBaseSyncPrimitives()
// task.
TEST(TaskSchedulerTaskTrackerWaitAllowedTest, WaitAllowed) {
  // Run the test on the separate thread since it is not possible to reset the
  // "wait allowed" bit of a thread without being a friend of
  // ThreadRestrictions.
  WaitAllowedTestThread wait_allowed_test_thread;
  wait_allowed_test_thread.Start();
  wait_allowed_test_thread.Join();
}

// Verify that TaskScheduler.TaskLatency.* histograms are correctly recorded
// when a task runs.
TEST(TaskSchedulerTaskTrackerHistogramTest, TaskLatency) {
  auto statistics_recorder = StatisticsRecorder::CreateTemporaryForTesting();

  TaskTracker tracker;
  MockCanScheduleSequenceObserver observer;

  struct {
    const TaskTraits traits;
    const char* const expected_histogram;
  } tests[] = {
      {{TaskPriority::BACKGROUND},
       "TaskScheduler.TaskLatencyMicroseconds.BackgroundTaskPriority"},
      {{MayBlock(), TaskPriority::BACKGROUND},
       "TaskScheduler.TaskLatencyMicroseconds.BackgroundTaskPriority.MayBlock"},
      {{WithBaseSyncPrimitives(), TaskPriority::BACKGROUND},
       "TaskScheduler.TaskLatencyMicroseconds.BackgroundTaskPriority.MayBlock"},
      {{TaskPriority::USER_VISIBLE},
       "TaskScheduler.TaskLatencyMicroseconds.UserVisibleTaskPriority"},
      {{MayBlock(), TaskPriority::USER_VISIBLE},
       "TaskScheduler.TaskLatencyMicroseconds.UserVisibleTaskPriority."
       "MayBlock"},
      {{WithBaseSyncPrimitives(), TaskPriority::USER_VISIBLE},
       "TaskScheduler.TaskLatencyMicroseconds.UserVisibleTaskPriority."
       "MayBlock"},
      {{TaskPriority::USER_BLOCKING},
       "TaskScheduler.TaskLatencyMicroseconds.UserBlockingTaskPriority"},
      {{MayBlock(), TaskPriority::USER_BLOCKING},
       "TaskScheduler.TaskLatencyMicroseconds.UserBlockingTaskPriority."
       "MayBlock"},
      {{WithBaseSyncPrimitives(), TaskPriority::USER_BLOCKING},
       "TaskScheduler.TaskLatencyMicroseconds.UserBlockingTaskPriority."
       "MayBlock"}};

  for (const auto& test : tests) {
    auto task = std::make_unique<Task>(FROM_HERE, Bind(&DoNothing), test.traits,
                                       TimeDelta());
    ASSERT_TRUE(tracker.WillPostTask(task.get()));

    HistogramTester tester;

    auto sequence = tracker.WillScheduleSequence(
        test::CreateSequenceWithTask(std::move(task)), &observer);
    ASSERT_TRUE(sequence);
    tracker.RunNextTask(std::move(sequence), &observer);
    tester.ExpectTotalCount(test.expected_histogram, 1);
  }
}

}  // namespace internal
}  // namespace base
