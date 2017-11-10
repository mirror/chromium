// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/delayed_task_manager.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/task_scheduler/task.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/test/test_simple_task_runner.h"
#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {
namespace {

constexpr TimeDelta kLongDelay = TimeDelta::FromHours(1);

class MockTaskTarget {
 public:
  MockTaskTarget() = default;
  ~MockTaskTarget() = default;

  // Use the |single_thread_task_runner_ref| as a way to determine if we
  // received the expected task.
  MOCK_METHOD1(DoPostTaskNowCallback, void(const SingleThreadTaskRunner*));

  void PostTaskNowCallback(Task task) {
    DoPostTaskNowCallback(task.single_thread_task_runner_ref.get());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockTaskTarget);
};

class TaskSchedulerDelayedTaskManagerTest : public testing::Test {
 protected:
  TaskSchedulerDelayedTaskManagerTest()
      : delayed_task_manager_(service_thread_task_runner_->GetMockTickClock()),
        task_(FROM_HERE, BindOnce(&DoNothing), TaskTraits(), kLongDelay) {
    task_.single_thread_task_runner_ref = task_runner_ref_;

    // The constructor of Task computes |delayed_run_time| by adding |delay| to
    // the real time. Recompute it by adding |delay| to the mock time.
    task_.delayed_run_time =
        service_thread_task_runner_->GetMockTickClock()->NowTicks() +
        kLongDelay;
  }
  ~TaskSchedulerDelayedTaskManagerTest() override = default;

  testing::StrictMock<MockTaskTarget> task_target_;
  const scoped_refptr<TestMockTimeTaskRunner> service_thread_task_runner_ =
      MakeRefCounted<TestMockTimeTaskRunner>();
  DelayedTaskManager delayed_task_manager_;
  Task task_;
  const scoped_refptr<SingleThreadTaskRunner> task_runner_ref_ =
      MakeRefCounted<TestSimpleTaskRunner>();

 private:
  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerDelayedTaskManagerTest);
};

}  // namespace

// Verify that a delayed task isn't forwarded before Start().
TEST_F(TaskSchedulerDelayedTaskManagerTest, DelayedTaskDoesNotRunBeforeStart) {
  // Send |task| to the DelayedTaskManager.
  delayed_task_manager_.AddDelayedTask(
      std::move(task_), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                 Unretained(&task_target_)));

  // Fast-forward time until the task is ripe for execution. Since Start() has
  // not been called, the task should be forwarded to |task_target_|
  // (|task_target_| is a StrictMock without expectations, so the test will fail
  // if the task is forwarded to it).
  service_thread_task_runner_->FastForwardBy(kLongDelay);
}

// Verify that a delayed task added before Start() and whose delay expires after
// Start() is forwarded when its delay expires.
TEST_F(TaskSchedulerDelayedTaskManagerTest,
       DelayedTaskPostedBeforeStartExpiresAfterStartRunsOnExpire) {
  // Send |task| to the DelayedTaskManager.
  delayed_task_manager_.AddDelayedTask(
      std::move(task_), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                 Unretained(&task_target_)));

  delayed_task_manager_.Start(service_thread_task_runner_);

  // Run tasks on the service thread. Don't expect any forwarding to
  // |task_target_| since the task isn't ripe for execution.
  service_thread_task_runner_->RunUntilIdle();

  // Fast-forward time until the task is ripe for execution. Expect the task to
  // be forwarded to |task_target_|.
  EXPECT_CALL(task_target_, DoPostTaskNowCallback(task_runner_ref_.get()));
  service_thread_task_runner_->FastForwardBy(kLongDelay);
}

// Verify that a delayed task added before Start() and whose delay expires
// before Start() is forwarded when Start() is called.
TEST_F(TaskSchedulerDelayedTaskManagerTest,
       DelayedTaskPostedBeforeStartExpiresBeforeStartRunsOnStart) {
  // Send |task| to the DelayedTaskManager.
  delayed_task_manager_.AddDelayedTask(
      std::move(task_), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                 Unretained(&task_target_)));

  // Run tasks on the service thread. Don't expect any forwarding to
  // |task_target_| since the task isn't ripe for execution.
  service_thread_task_runner_->RunUntilIdle();

  // Fast-forward time until the task is ripe for execution. Don't expect the
  // task to be forwarded since Start() hasn't been called yet.
  service_thread_task_runner_->FastForwardBy(kLongDelay);

  // Start the DelayedTaskManager. Expect the task to be forwarded to
  // |task_target_|.
  EXPECT_CALL(task_target_, DoPostTaskNowCallback(task_runner_ref_.get()));
  delayed_task_manager_.Start(service_thread_task_runner_);
  service_thread_task_runner_->RunUntilIdle();
}

// Verify that a delayed task added after Start() isn't forwarded before it is
// ripe for execution.
TEST_F(TaskSchedulerDelayedTaskManagerTest, DelayedTaskDoesNotRunTooEarly) {
  delayed_task_manager_.Start(service_thread_task_runner_);

  // Send |task| to the DelayedTaskManager.
  delayed_task_manager_.AddDelayedTask(
      std::move(task_), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                 Unretained(&task_target_)));

  // Run tasks that are ripe for execution. Don't expect any forwarding to
  // |task_target_|.
  service_thread_task_runner_->RunUntilIdle();
}

// Verify that a delayed task added after Start() is forwarded when it is ripe
// for execution.
TEST_F(TaskSchedulerDelayedTaskManagerTest, DelayedTaskRunsAfterDelay) {
  delayed_task_manager_.Start(service_thread_task_runner_);

  // Send |task| to the DelayedTaskManager.
  delayed_task_manager_.AddDelayedTask(
      std::move(task_), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                 Unretained(&task_target_)));

  // Fast-forward time. Expect the task is forwarded to |task_target_|.
  EXPECT_CALL(task_target_, DoPostTaskNowCallback(task_runner_ref_.get()));
  service_thread_task_runner_->FastForwardBy(kLongDelay);
}

// Verify that multiple delayed tasks added after Start() are forwarded when
// they are ripe for execution.
TEST_F(TaskSchedulerDelayedTaskManagerTest, DelayedTasksRunAfterDelay) {
  delayed_task_manager_.Start(service_thread_task_runner_);
  Task task_a(FROM_HERE, BindOnce(&DoNothing), TaskTraits(),
              TimeDelta::FromHours(1));
  auto task_runner_ref_a = MakeRefCounted<TestSimpleTaskRunner>();
  task_a.single_thread_task_runner_ref = task_runner_ref_a;

  Task task_b(FROM_HERE, BindOnce(&DoNothing), TaskTraits(),
              TimeDelta::FromHours(2));
  auto task_runner_ref_b = MakeRefCounted<TestSimpleTaskRunner>();
  task_b.single_thread_task_runner_ref = task_runner_ref_b;

  Task task_c(FROM_HERE, BindOnce(&DoNothing), TaskTraits(),
              TimeDelta::FromHours(1));
  auto task_runner_ref_c = MakeRefCounted<TestSimpleTaskRunner>();
  task_c.single_thread_task_runner_ref = task_runner_ref_c;

  // Send tasks to the DelayedTaskManager.
  delayed_task_manager_.AddDelayedTask(
      std::move(task_a), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                  Unretained(&task_target_)));
  delayed_task_manager_.AddDelayedTask(
      std::move(task_b), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                  Unretained(&task_target_)));
  delayed_task_manager_.AddDelayedTask(
      std::move(task_c), BindOnce(&MockTaskTarget::PostTaskNowCallback,
                                  Unretained(&task_target_)));

  // Run tasks that are ripe for execution on the service thread. Don't expect
  // any call to |task_target_|.
  service_thread_task_runner_->RunUntilIdle();

  // Fast-forward time. Expect |task_a_raw| and |task_c_raw| to be forwarded to
  // |task_target_|.
  EXPECT_CALL(task_target_, DoPostTaskNowCallback(task_runner_ref_a.get()));
  EXPECT_CALL(task_target_, DoPostTaskNowCallback(task_runner_ref_c.get()));
  service_thread_task_runner_->FastForwardBy(TimeDelta::FromHours(1));
  testing::Mock::VerifyAndClear(&task_target_);

  // Fast-forward time. Expect |task_b_raw| to be forwarded to |task_target_|.
  EXPECT_CALL(task_target_, DoPostTaskNowCallback(task_runner_ref_b.get()));
  service_thread_task_runner_->FastForwardBy(TimeDelta::FromHours(1));
  testing::Mock::VerifyAndClear(&task_target_);
}

}  // namespace internal
}  // namespace base
