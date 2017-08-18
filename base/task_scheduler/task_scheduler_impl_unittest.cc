// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/task_scheduler_impl.h"

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/scheduler_worker_pool_params.h"
#include "base/task_scheduler/task_tracker.h"
#include "base/task_scheduler/task_traits.h"
#include "base/task_scheduler/test_task_factory.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequence_local_storage_slot.h"
#include "base/threading/simple_thread.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX)
#include <unistd.h>

#include "base/debug/leak_annotations.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/file_util.h"
#include "base/posix/eintr_wrapper.h"
#endif  // defined(OS_POSIX)

#if defined(OS_WIN)
#include "base/win/com_init_util.h"
#endif  // defined(OS_WIN)

namespace base {
namespace internal {

namespace {

struct TaskSchedulerImplTestParams {
  TaskSchedulerImplTestParams(const TaskTraits& traits,
                              test::ExecutionMode execution_mode,
                              bool run_all_tasks_with_user_blocking_priority)
      : traits(traits),
        execution_mode(execution_mode),
        run_all_tasks_with_user_blocking_priority(
            run_all_tasks_with_user_blocking_priority) {}

  TaskTraits traits;
  test::ExecutionMode execution_mode;
  bool run_all_tasks_with_user_blocking_priority;
};

#if DCHECK_IS_ON()
// Returns whether I/O calls are allowed on the current thread.
bool GetIOAllowed() {
  const bool previous_value = ThreadRestrictions::SetIOAllowed(true);
  ThreadRestrictions::SetIOAllowed(previous_value);
  return previous_value;
}
#endif

// Verify that the current thread priority and I/O restrictions are appropriate
// to run a Task with |traits|.
// Note: ExecutionMode is verified inside TestTaskFactory.
void VerifyTaskEnvironment(const TaskTraits& traits) {
  const bool supports_background_priority =
      Lock::HandlesMultipleThreadPriorities() &&
      PlatformThread::CanIncreaseCurrentThreadPriority();

  DCHECK_EQ(supports_background_priority &&
                    traits.priority() == TaskPriority::BACKGROUND
                ? ThreadPriority::BACKGROUND
                : ThreadPriority::NORMAL,
            PlatformThread::GetCurrentThreadPriority());

#if DCHECK_IS_ON()
  // The #if above is required because GetIOAllowed() always returns true when
  // !DCHECK_IS_ON(), even when |traits| don't allow file I/O.
  EXPECT_EQ(traits.may_block(), GetIOAllowed());
#endif

  // Verify that the thread the task is running on is named as expected.
  const std::string current_thread_name(PlatformThread::GetName());
  EXPECT_NE(std::string::npos, current_thread_name.find("TaskScheduler"));
  EXPECT_NE(std::string::npos,
            current_thread_name.find(
                traits.priority() == TaskPriority::BACKGROUND ? "Background"
                                                              : "Foreground"));
  EXPECT_EQ(traits.may_block(),
            current_thread_name.find("Blocking") != std::string::npos);
}

void VerifyTaskEnvironmentAndSignalEvent(const TaskTraits& traits,
                                         WaitableEvent* event) {
  DCHECK(event);
  VerifyTaskEnvironment(traits);
  event->Signal();
}

void VerifyTimeAndTaskEnvironmentAndSignalEvent(const TaskTraits& traits,
                                                TimeTicks expected_time,
                                                WaitableEvent* event) {
  DCHECK(event);
  EXPECT_LE(expected_time, TimeTicks::Now());
  VerifyTaskEnvironment(traits);
  event->Signal();
}

scoped_refptr<TaskRunner> CreateTaskRunnerWithTraitsAndExecutionMode(
    TaskScheduler* scheduler,
    const TaskTraits& traits,
    test::ExecutionMode execution_mode) {
  switch (execution_mode) {
    case test::ExecutionMode::PARALLEL:
      return scheduler->CreateTaskRunnerWithTraits(traits);
    case test::ExecutionMode::SEQUENCED:
      return scheduler->CreateSequencedTaskRunnerWithTraits(traits);
    case test::ExecutionMode::SINGLE_THREADED: {
      return scheduler->CreateSingleThreadTaskRunnerWithTraits(
          traits, SingleThreadTaskRunnerThreadMode::SHARED);
    }
  }
  ADD_FAILURE() << "Unknown ExecutionMode";
  return nullptr;
}

// Returns a vector with a TaskSchedulerImplTestParams for each valid
// combination of {ExecutionMode, TaskPriority, MayBlock(),
// run_all_tasks_with_user_blocking_priority}.
std::vector<TaskSchedulerImplTestParams> GetTaskSchedulerImplTestParams() {
  std::vector<TaskSchedulerImplTestParams> params;

  const test::ExecutionMode execution_modes[] = {
      test::ExecutionMode::PARALLEL, test::ExecutionMode::SEQUENCED,
      test::ExecutionMode::SINGLE_THREADED};
  const bool run_all_tasks_with_user_blocking_priority_options[] = {false,
                                                                    true};

  for (test::ExecutionMode execution_mode : execution_modes) {
    for (size_t priority_index = static_cast<size_t>(TaskPriority::LOWEST);
         priority_index <= static_cast<size_t>(TaskPriority::HIGHEST);
         ++priority_index) {
      for (bool run_all_tasks_with_user_blocking_priority :
           run_all_tasks_with_user_blocking_priority_options) {
        const TaskPriority priority = static_cast<TaskPriority>(priority_index);
        params.push_back(TaskSchedulerImplTestParams(
            {priority}, execution_mode,
            run_all_tasks_with_user_blocking_priority));
        params.push_back(TaskSchedulerImplTestParams(
            {MayBlock(), priority}, execution_mode,
            run_all_tasks_with_user_blocking_priority));
      }
    }
  }

  return params;
}

void StartTaskScheduler(TaskSchedulerImpl* scheduler) {
  constexpr TimeDelta kSuggestedReclaimTime = TimeDelta::FromSeconds(30);
  constexpr int kMaxNumBackgroundThreads = 1;
  constexpr int kMaxNumBackgroundBlockingThreads = 3;
  constexpr int kMaxNumForegroundThreads = 4;
  constexpr int kMaxNumForegroundBlockingThreads = 12;

  scheduler->Start({{kMaxNumBackgroundThreads, kSuggestedReclaimTime},
                    {kMaxNumBackgroundBlockingThreads, kSuggestedReclaimTime},
                    {kMaxNumForegroundThreads, kSuggestedReclaimTime},
                    {kMaxNumForegroundBlockingThreads, kSuggestedReclaimTime}});
  }

  TaskTraits GetExpectedTraitsInEnvironment(
      TaskSchedulerImplTestParams params) {
    return params.run_all_tasks_with_user_blocking_priority
               ? TaskTraits::Override(params.traits,
                                      TaskPriority::USER_BLOCKING)
               : params.traits;
  }

  class TaskSchedulerImplParameterizedTest
      : public testing::TestWithParam<TaskSchedulerImplTestParams> {
   protected:
    TaskSchedulerImplParameterizedTest()
        : scheduler_("Test",
                     std::make_unique<TaskTracker>(),
                     GetParam().run_all_tasks_with_user_blocking_priority) {}

    void TearDown() override {
      scheduler_.FlushForTesting();
      scheduler_.JoinForTesting();
    }

    TaskSchedulerImpl scheduler_;

   private:
    DISALLOW_COPY_AND_ASSIGN(TaskSchedulerImplParameterizedTest);
};

}  // namespace

// Verifies that a Task posted via PostDelayedTaskWithTraits with parameterized
// TaskTraits and no delay runs on a thread with the expected priority and I/O
// restrictions. The ExecutionMode parameter is ignored by this test.
TEST_P(TaskSchedulerImplParameterizedTest, PostDelayedTaskWithTraitsNoDelay) {
  StartTaskScheduler(&scheduler_);
  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  scheduler_.PostDelayedTaskWithTraits(
      FROM_HERE, GetParam().traits,
      BindOnce(&VerifyTaskEnvironmentAndSignalEvent,
               GetExpectedTraitsInEnvironment(GetParam()),
               Unretained(&task_ran)),
      TimeDelta());
  task_ran.Wait();
}

// Verifies that a Task posted via PostDelayedTaskWithTraits with parameterized
// TaskTraits and a non-zero delay runs on a thread with the expected priority
// and I/O restrictions after the delay expires. The ExecutionMode parameter is
// ignored by this test.
TEST_P(TaskSchedulerImplParameterizedTest, PostDelayedTaskWithTraitsWithDelay) {
  StartTaskScheduler(&scheduler_);
  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  scheduler_.PostDelayedTaskWithTraits(
      FROM_HERE, GetParam().traits,
      BindOnce(&VerifyTimeAndTaskEnvironmentAndSignalEvent,
               GetExpectedTraitsInEnvironment(GetParam()),
               TimeTicks::Now() + TestTimeouts::tiny_timeout(),
               Unretained(&task_ran)),
      TestTimeouts::tiny_timeout());
  task_ran.Wait();
}

// Verifies that Tasks posted via a TaskRunner with parameterized TaskTraits and
// ExecutionMode run on a thread with the expected priority and I/O restrictions
// and respect the characteristics of their ExecutionMode.
TEST_P(TaskSchedulerImplParameterizedTest, PostTasksViaTaskRunner) {
  StartTaskScheduler(&scheduler_);
  test::TestTaskFactory factory(
      CreateTaskRunnerWithTraitsAndExecutionMode(&scheduler_, GetParam().traits,
                                                 GetParam().execution_mode),
      GetParam().execution_mode);
  EXPECT_FALSE(factory.task_runner()->RunsTasksInCurrentSequence());

  const size_t kNumTasksPerTest = 150;
  for (size_t i = 0; i < kNumTasksPerTest; ++i) {
    factory.PostTask(test::TestTaskFactory::PostNestedTask::NO,
                     Bind(&VerifyTaskEnvironment,
                          GetExpectedTraitsInEnvironment(GetParam())));
  }

  factory.WaitForAllTasksToRun();
}

// Verifies that a task posted via PostDelayedTaskWithTraits without a delay
// doesn't run before Start() is called.
TEST_P(TaskSchedulerImplParameterizedTest,
       PostDelayedTaskWithTraitsNoDelayBeforeStart) {
  WaitableEvent task_running(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  scheduler_.PostDelayedTaskWithTraits(
      FROM_HERE, GetParam().traits,
      BindOnce(&VerifyTaskEnvironmentAndSignalEvent,
               GetExpectedTraitsInEnvironment(GetParam()),
               Unretained(&task_running)),
      TimeDelta());

  // Wait a little bit to make sure that the task isn't scheduled before
  // Start(). Note: This test won't catch a case where the task runs just after
  // the check and before Start(). However, we expect the test to be flaky if
  // the tested code allows that to happen.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  EXPECT_FALSE(task_running.IsSignaled());

  StartTaskScheduler(&scheduler_);
  task_running.Wait();
}

// Verifies that a task posted via PostDelayedTaskWithTraits with a delay
// doesn't run before Start() is called.
TEST_P(TaskSchedulerImplParameterizedTest,
       PostDelayedTaskWithTraitsWithDelayBeforeStart) {
  WaitableEvent task_running(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  scheduler_.PostDelayedTaskWithTraits(
      FROM_HERE, GetParam().traits,
      BindOnce(&VerifyTimeAndTaskEnvironmentAndSignalEvent,
               GetExpectedTraitsInEnvironment(GetParam()),
               TimeTicks::Now() + TestTimeouts::tiny_timeout(),
               Unretained(&task_running)),
      TestTimeouts::tiny_timeout());

  // Wait a little bit to make sure that the task isn't scheduled before
  // Start(). Note: This test won't catch a case where the task runs just after
  // the check and before Start(). However, we expect the test to be flaky if
  // the tested code allows that to happen.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  EXPECT_FALSE(task_running.IsSignaled());

  StartTaskScheduler(&scheduler_);
  task_running.Wait();
}

// Verifies that a task posted via a TaskRunner doesn't run before Start() is
// called.
TEST_P(TaskSchedulerImplParameterizedTest, PostTaskViaTaskRunnerBeforeStart) {
  WaitableEvent task_running(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
  CreateTaskRunnerWithTraitsAndExecutionMode(&scheduler_, GetParam().traits,
                                             GetParam().execution_mode)
      ->PostTask(FROM_HERE, BindOnce(&VerifyTaskEnvironmentAndSignalEvent,
                                     GetExpectedTraitsInEnvironment(GetParam()),
                                     Unretained(&task_running)));

  // Wait a little bit to make sure that the task isn't scheduled before
  // Start(). Note: This test won't catch a case where the task runs just after
  // the check and before Start(). However, we expect the test to be flaky if
  // the tested code allows that to happen.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  EXPECT_FALSE(task_running.IsSignaled());

  StartTaskScheduler(&scheduler_);

  // This should not hang if the task is scheduled after Start().
  task_running.Wait();
}

INSTANTIATE_TEST_CASE_P(OneTaskSchedulerImplTestParams,
                        TaskSchedulerImplParameterizedTest,
                        ::testing::ValuesIn(GetTaskSchedulerImplTestParams()));

namespace {

class ThreadPostingTasks : public SimpleThread {
 public:
  // Creates a thread that posts Tasks to |scheduler| with |traits| and
  // |execution_mode|.
  ThreadPostingTasks(TaskSchedulerImpl* scheduler,
                     const TaskTraits& posting_traits,
                     const TaskTraits& expected_traits_in_environment,
                     test::ExecutionMode execution_mode)
      : SimpleThread("ThreadPostingTasks"),
        expected_traits_in_environment_(expected_traits_in_environment),
        factory_(CreateTaskRunnerWithTraitsAndExecutionMode(scheduler,
                                                            posting_traits,
                                                            execution_mode),
                 execution_mode) {}

  void WaitForAllTasksToRun() { factory_.WaitForAllTasksToRun(); }

 private:
  void Run() override {
    EXPECT_FALSE(factory_.task_runner()->RunsTasksInCurrentSequence());

    const size_t kNumTasksPerThread = 150;
    for (size_t i = 0; i < kNumTasksPerThread; ++i) {
      factory_.PostTask(
          test::TestTaskFactory::PostNestedTask::NO,
          Bind(&VerifyTaskEnvironment, expected_traits_in_environment_));
    }
  }

  const TaskTraits expected_traits_in_environment_;
  test::TestTaskFactory factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadPostingTasks);
};

class TaskSchedulerImplTest : public testing::Test {
 protected:
  TaskSchedulerImplTest() : scheduler_("Test") {}

  void TearDown() override {
    scheduler_.FlushForTesting();
    scheduler_.JoinForTesting();
  }

  TaskSchedulerImpl scheduler_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerImplTest);
};

}  // namespace

// Spawns threads that simultaneously post Tasks to TaskRunners with various
// TaskTraits and ExecutionModes. Verifies that each Task runs on a thread with
// the expected priority and I/O restrictions and respects the characteristics
// of its ExecutionMode.
TEST_F(TaskSchedulerImplTest, MultipleTaskSchedulerImplTestParams) {
  StartTaskScheduler(&scheduler_);
  std::vector<std::unique_ptr<ThreadPostingTasks>> threads_posting_tasks;
  for (const auto& params : GetTaskSchedulerImplTestParams()) {
    // This test ignores |params.run_all_tasks_with_user_blocking_priority|.
    threads_posting_tasks.push_back(std::make_unique<ThreadPostingTasks>(
        &scheduler_, params.traits, params.traits, params.execution_mode));
    threads_posting_tasks.back()->Start();
  }

  for (const auto& thread : threads_posting_tasks) {
    thread->WaitForAllTasksToRun();
    thread->Join();
  }
}

TEST_F(TaskSchedulerImplTest, GetMaxConcurrentTasksWithTraitsDeprecated) {
  StartTaskScheduler(&scheduler_);
  EXPECT_EQ(1, scheduler_.GetMaxConcurrentTasksWithTraitsDeprecated(
                   {TaskPriority::BACKGROUND}));
  EXPECT_EQ(3, scheduler_.GetMaxConcurrentTasksWithTraitsDeprecated(
                   {MayBlock(), TaskPriority::BACKGROUND}));
  EXPECT_EQ(4, scheduler_.GetMaxConcurrentTasksWithTraitsDeprecated(
                   {TaskPriority::USER_VISIBLE}));
  EXPECT_EQ(12, scheduler_.GetMaxConcurrentTasksWithTraitsDeprecated(
                    {MayBlock(), TaskPriority::USER_VISIBLE}));
  EXPECT_EQ(4, scheduler_.GetMaxConcurrentTasksWithTraitsDeprecated(
                   {TaskPriority::USER_BLOCKING}));
  EXPECT_EQ(12, scheduler_.GetMaxConcurrentTasksWithTraitsDeprecated(
                    {MayBlock(), TaskPriority::USER_BLOCKING}));
}

// Verify that the RunsTasksInCurrentSequence() method of a SequencedTaskRunner
// returns false when called from a task that isn't part of the sequence.
TEST_F(TaskSchedulerImplTest, SequencedRunsTasksInCurrentSequence) {
  StartTaskScheduler(&scheduler_);
  auto single_thread_task_runner =
      scheduler_.CreateSingleThreadTaskRunnerWithTraits(
          TaskTraits(), SingleThreadTaskRunnerThreadMode::SHARED);
  auto sequenced_task_runner =
      scheduler_.CreateSequencedTaskRunnerWithTraits(TaskTraits());

  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  single_thread_task_runner->PostTask(
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

// Verify that the RunsTasksInCurrentSequence() method of a
// SingleThreadTaskRunner returns false when called from a task that isn't part
// of the sequence.
TEST_F(TaskSchedulerImplTest, SingleThreadRunsTasksInCurrentSequence) {
  StartTaskScheduler(&scheduler_);
  auto sequenced_task_runner =
      scheduler_.CreateSequencedTaskRunnerWithTraits(TaskTraits());
  auto single_thread_task_runner =
      scheduler_.CreateSingleThreadTaskRunnerWithTraits(
          TaskTraits(), SingleThreadTaskRunnerThreadMode::SHARED);

  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  sequenced_task_runner->PostTask(
      FROM_HERE,
      BindOnce(
          [](scoped_refptr<TaskRunner> single_thread_task_runner,
             WaitableEvent* task_ran) {
            EXPECT_FALSE(
                single_thread_task_runner->RunsTasksInCurrentSequence());
            task_ran->Signal();
          },
          single_thread_task_runner, Unretained(&task_ran)));
  task_ran.Wait();
}

#if defined(OS_WIN)
TEST_F(TaskSchedulerImplTest, COMSTATaskRunnersRunWithCOMSTA) {
  StartTaskScheduler(&scheduler_);
  auto com_sta_task_runner = scheduler_.CreateCOMSTATaskRunnerWithTraits(
      TaskTraits(), SingleThreadTaskRunnerThreadMode::SHARED);

  WaitableEvent task_ran(WaitableEvent::ResetPolicy::MANUAL,
                         WaitableEvent::InitialState::NOT_SIGNALED);
  com_sta_task_runner->PostTask(
      FROM_HERE, Bind(
                     [](WaitableEvent* task_ran) {
                       win::AssertComApartmentType(win::ComApartmentType::STA);
                       task_ran->Signal();
                     },
                     Unretained(&task_ran)));
  task_ran.Wait();
}
#endif  // defined(OS_WIN)

TEST_F(TaskSchedulerImplTest, DelayedTasksNotRunAfterShutdown) {
  StartTaskScheduler(&scheduler_);
  // As with delayed tasks in general, this is racy. If the task does happen to
  // run after Shutdown within the timeout, it will fail this test.
  //
  // The timeout should be set sufficiently long enough to ensure that the
  // delayed task did not run. 2x is generally good enough.
  //
  // A non-racy way to do this would be to post two sequenced tasks:
  // 1) Regular Post Task: A WaitableEvent.Wait
  // 2) Delayed Task: ADD_FAILURE()
  // and signalling the WaitableEvent after Shutdown() on a different thread
  // since Shutdown() will block. However, the cost of managing this extra
  // thread was deemed to be too great for the unlikely race.
  scheduler_.PostDelayedTaskWithTraits(FROM_HERE, TaskTraits(),
                                       BindOnce([]() { ADD_FAILURE(); }),
                                       TestTimeouts::tiny_timeout());
  scheduler_.Shutdown();
  PlatformThread::Sleep(TestTimeouts::tiny_timeout() * 2);
}

#if defined(OS_POSIX)

TEST_F(TaskSchedulerImplTest, FileDescriptorWatcherNoOpsAfterShutdown) {
  StartTaskScheduler(&scheduler_);

  int pipes[2];
  ASSERT_EQ(0, pipe(pipes));

  scoped_refptr<TaskRunner> blocking_task_runner =
      scheduler_.CreateSequencedTaskRunnerWithTraits(
          {TaskShutdownBehavior::BLOCK_SHUTDOWN});
  blocking_task_runner->PostTask(
      FROM_HERE,
      BindOnce(
          [](int read_fd) {
            std::unique_ptr<FileDescriptorWatcher::Controller> controller =
                FileDescriptorWatcher::WatchReadable(
                    read_fd, BindRepeating([]() { NOTREACHED(); }));

            // This test is for components that intentionally leak their
            // watchers at shutdown. We can't clean |controller| up because its
            // destructor will assert that it's being called from the correct
            // sequence. After the task scheduler is shutdown, it is not
            // possible to run tasks on this sequence.
            //
            // Note: Do not inline the controller.release() call into the
            //       ANNOTATE_LEAKING_OBJECT_PTR as the annotation is removed
            //       by the preprocessor in non-LEAK_SANITIZER builds,
            //       effectively breaking this test.
            ANNOTATE_LEAKING_OBJECT_PTR(controller.get());
            controller.release();
          },
          pipes[0]));

  scheduler_.Shutdown();

  constexpr char kByte = '!';
  ASSERT_TRUE(WriteFileDescriptor(pipes[1], &kByte, sizeof(kByte)));

  // Give a chance for the file watcher to fire before closing the handles.
  PlatformThread::Sleep(TestTimeouts::tiny_timeout());

  EXPECT_EQ(0, IGNORE_EINTR(close(pipes[0])));
  EXPECT_EQ(0, IGNORE_EINTR(close(pipes[1])));
}
#endif  // defined(OS_POSIX)

// Verify that tasks posted on the same sequence access the same values on
// SequenceLocalStorage, and tasks on different sequences see different values.
TEST_F(TaskSchedulerImplTest, SequenceLocalStorage) {
  StartTaskScheduler(&scheduler_);

  SequenceLocalStorageSlot<int> slot;
  auto sequenced_task_runner1 =
      scheduler_.CreateSequencedTaskRunnerWithTraits(TaskTraits());
  auto sequenced_task_runner2 =
      scheduler_.CreateSequencedTaskRunnerWithTraits(TaskTraits());

  sequenced_task_runner1->PostTask(
      FROM_HERE,
      BindOnce([](SequenceLocalStorageSlot<int>* slot) { slot->Set(11); },
               &slot));

  sequenced_task_runner1->PostTask(FROM_HERE,
                                   BindOnce(
                                       [](SequenceLocalStorageSlot<int>* slot) {
                                         EXPECT_EQ(slot->Get(), 11);
                                       },
                                       &slot));

  sequenced_task_runner2->PostTask(FROM_HERE,
                                   BindOnce(
                                       [](SequenceLocalStorageSlot<int>* slot) {
                                         EXPECT_NE(slot->Get(), 11);
                                       },
                                       &slot));

  scheduler_.FlushForTesting();
}

}  // namespace internal
}  // namespace base
