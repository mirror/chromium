// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/test_mock_time_task_runner.h"

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/gtest_util.h"
#include "base/test/test_timeouts.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class TestMockTimeTaskRunnerTestWithParam
    : public testing::TestWithParam<TestMockTimeTaskRunner::Type> {
 public:
  void SetUp() override {
    // Instantiate a pre-existing Delegate to test overriding in the
    // kTakeOverThread test case.
    if (GetParam() == TestMockTimeTaskRunner::Type::kTakeOverThread)
      to_be_overridden = std::make_unique<MessageLoopForUI>();
  }

 private:
  std::unique_ptr<RunLoop::Delegate> to_be_overridden;
};

// Basic usage should work the same from default and bound
// TestMockTimeTaskRunners.
TEST_P(TestMockTimeTaskRunnerTestWithParam, Basic) {
  auto mock_time_task_runner =
      MakeRefCounted<TestMockTimeTaskRunner>(GetParam());
  int counter = 0;

  mock_time_task_runner->PostTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 1; }, Unretained(&counter)));
  mock_time_task_runner->PostTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 32; }, Unretained(&counter)));
  mock_time_task_runner->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 256; }, Unretained(&counter)),
      TimeDelta::FromSeconds(3));
  mock_time_task_runner->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 64; }, Unretained(&counter)),
      TimeDelta::FromSeconds(1));
  mock_time_task_runner->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 1024; }, Unretained(&counter)),
      TimeDelta::FromMinutes(20));
  mock_time_task_runner->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 4096; }, Unretained(&counter)),
      TimeDelta::FromDays(20));

  int expected_value = 0;
  EXPECT_EQ(expected_value, counter);
  mock_time_task_runner->RunUntilIdle();
  expected_value += 1;
  expected_value += 32;
  EXPECT_EQ(expected_value, counter);

  mock_time_task_runner->RunUntilIdle();
  EXPECT_EQ(expected_value, counter);

  mock_time_task_runner->FastForwardBy(TimeDelta::FromSeconds(1));
  expected_value += 64;
  EXPECT_EQ(expected_value, counter);

  mock_time_task_runner->FastForwardBy(TimeDelta::FromSeconds(5));
  expected_value += 256;
  EXPECT_EQ(expected_value, counter);

  mock_time_task_runner->FastForwardUntilNoTasksRemain();
  expected_value += 1024;
  expected_value += 4096;
  EXPECT_EQ(expected_value, counter);
}

// A default TestMockTimeTaskRunner shouldn't result in a thread association.
TEST(TestMockTimeTaskRunnerTest, DefaultUnbound) {
  auto unbound_mock_time_task_runner = MakeRefCounted<TestMockTimeTaskRunner>();
  EXPECT_FALSE(ThreadTaskRunnerHandle::IsSet());
  EXPECT_FALSE(SequencedTaskRunnerHandle::IsSet());
  EXPECT_DCHECK_DEATH({ RunLoop().RunUntilIdle(); });
}

TEST_P(TestMockTimeTaskRunnerTestWithParam, RunLoopDriveableWhenBound) {
  if (GetParam() == TestMockTimeTaskRunner::Type::kStandalone) {
    // This test doesn't apply to the kStandalone mode which can't RunLoop.
    EXPECT_DCHECK_DEATH({ RunLoop().RunUntilIdle(); });
    return;
  }

  auto bound_mock_time_task_runner =
      MakeRefCounted<TestMockTimeTaskRunner>(GetParam());

  int counter = 0;
  ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 1; }, Unretained(&counter)));
  ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 32; }, Unretained(&counter)));
  ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 256; }, Unretained(&counter)),
      TimeDelta::FromSeconds(3));
  ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 64; }, Unretained(&counter)),
      TimeDelta::FromSeconds(1));
  ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 1024; }, Unretained(&counter)),
      TimeDelta::FromMinutes(20));
  ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      BindOnce([](int* counter) { *counter += 4096; }, Unretained(&counter)),
      TimeDelta::FromDays(20));

  int expected_value = 0;
  EXPECT_EQ(expected_value, counter);
  RunLoop().RunUntilIdle();
  expected_value += 1;
  expected_value += 32;
  EXPECT_EQ(expected_value, counter);

  RunLoop().RunUntilIdle();
  EXPECT_EQ(expected_value, counter);

  {
    RunLoop run_loop;
    ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(), TimeDelta::FromSeconds(1));
    ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        BindOnce([](int* counter) { *counter += 8192; }, Unretained(&counter)),
        TimeDelta::FromSeconds(1));

    // The QuitClosure() should be ordered between the 64 and the 8192
    // increments and should preempt the latter.
    run_loop.Run();
    expected_value += 64;
    EXPECT_EQ(expected_value, counter);

    // Running until idle should process the 8192 increment whose delay has
    // expired in the previous Run().
    RunLoop().RunUntilIdle();
    expected_value += 8192;
    EXPECT_EQ(expected_value, counter);
  }

  {
    RunLoop run_loop;
    ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitWhenIdleClosure(), TimeDelta::FromSeconds(5));
    ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        BindOnce([](int* counter) { *counter += 16384; }, Unretained(&counter)),
        TimeDelta::FromSeconds(5));

    // The QuitWhenIdleClosure() shouldn't preempt equally delayed tasks and as
    // such the 16384 increment should be processed before quitting.
    run_loop.Run();
    expected_value += 256;
    expected_value += 16384;
    EXPECT_EQ(expected_value, counter);
  }

  // Process the remaining tasks (note: do not mimic this elsewhere,
  // TestMockTimeTaskRunner::FastForwardUntilNoTasksRemain() is a better API to
  // do this, this is just done here for the purpose of extensively testing the
  // RunLoop approach).
  RunLoop run_loop;
  ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), TimeDelta::FromDays(50));

  run_loop.Run();
  expected_value += 1024;
  expected_value += 4096;
  EXPECT_EQ(expected_value, counter);
}

// Regression test that receiving the quit-when-idle signal when already empty
// works as intended (i.e. that |TestMockTimeTaskRunner::tasks_non_empty_cv_| is
// properly signaled in the kBoundToThread case and that |overridden_delegate_|
// is properly unblocked in the kTakeOverThread case).
TEST_P(TestMockTimeTaskRunnerTestWithParam, RunLoopQuitFromIdle) {
  if (GetParam() == TestMockTimeTaskRunner::Type::kStandalone) {
    // This test doesn't apply to the kStandalone mode which can't RunLoop.
    EXPECT_DCHECK_DEATH({ RunLoop().RunUntilIdle(); });
    return;
  }

  auto bound_mock_time_task_runner =
      MakeRefCounted<TestMockTimeTaskRunner>(GetParam());

  Thread quitting_thread("quitting thread");
  quitting_thread.Start();

  RunLoop run_loop;
  quitting_thread.task_runner()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), TestTimeouts::tiny_timeout());
  run_loop.Run();
}

INSTANTIATE_TEST_CASE_P(
    Standalone,
    TestMockTimeTaskRunnerTestWithParam,
    testing::Values(TestMockTimeTaskRunner::Type::kStandalone));
INSTANTIATE_TEST_CASE_P(
    BoundToThread,
    TestMockTimeTaskRunnerTestWithParam,
    testing::Values(TestMockTimeTaskRunner::Type::kBoundToThread));
INSTANTIATE_TEST_CASE_P(
    TakeOverThread,
    TestMockTimeTaskRunnerTestWithParam,
    testing::Values(TestMockTimeTaskRunner::Type::kTakeOverThread));

}  // namespace base
