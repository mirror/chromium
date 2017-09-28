// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "media/audio/alive_checker.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {
int kCheckIntervalMs = 10;
int kNotifyIntervalMs = 7;
int kTimeoutMs = 50;
}  // namespace

class MockPowerObserverHelper : public PowerObserverHelper {
 public:
  MockPowerObserverHelper(scoped_refptr<base::SequencedTaskRunner> task_runner,
                          base::RepeatingClosure suspend_callback,
                          base::RepeatingClosure resume_callback)

      : PowerObserverHelper(std::move(task_runner),
                            std::move(suspend_callback),
                            std::move(resume_callback)) {}

  bool IsSuspending() const override {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    return is_suspending_;
  }

  void Suspend() {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    is_suspending_ = true;
    suspend_callback_.Run();
  }

  void Resume() {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    is_suspending_ = false;
    resume_callback_.Run();
  }

 private:
  bool is_suspending_;
};

class AliveCheckerTest : public testing::Test {
 public:
  AliveCheckerTest()
      : alive_checker_thread_("AliveCheckerThread"),
        detected_dead_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                             base::WaitableEvent::InitialState::NOT_SIGNALED) {
    alive_checker_thread_.StartAndWaitForTesting();
  }

  void DetectedDead() {
    EXPECT_TRUE(alive_checker_thread_.task_runner()->BelongsToCurrentThread());
    detected_dead_event_.Signal();
  }

  std::unique_ptr<PowerObserverHelper> CreatePowerObserverHelper(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      base::RepeatingClosure suspend_callback,
      base::RepeatingClosure resume_callback) {
    std::unique_ptr<MockPowerObserverHelper> mock_power_observer_helper =
        std::make_unique<MockPowerObserverHelper>(std::move(task_runner),
                                                  std::move(suspend_callback),
                                                  std::move(resume_callback));
    mock_power_observer_helper_ = mock_power_observer_helper.get();
    return mock_power_observer_helper;
  }

 protected:
  ~AliveCheckerTest() override {}

  // Notifies |alive_checker_| that we're alive, and if
  // |remaining_notifications| > 1, posts a delayed task to itself on
  // |alive_checker_thread_| with |remaining_notifications| decreased by 1. Can
  // be called on any task runner.
  void NotifyAliveMultipleTimes(int remaining_notifications,
                                base::TimeDelta delay) {
    alive_checker_->NotifyAlive();
    if (remaining_notifications > 1) {
      alive_checker_thread_.task_runner()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&AliveCheckerTest::NotifyAliveMultipleTimes,
                     base::Unretained(this), remaining_notifications - 1,
                     delay),
          delay);
    }
  }

  void WaitUntilDetectedDead() {
    detected_dead_event_.Wait();
    detected_dead_event_.Reset();
  }

  // Returns true if detected dead, false if timed out.
  bool WaitUntilDetectedDeadWithTimeout(int timeout_ms) {
    bool signaled = detected_dead_event_.TimedWait(
        base::TimeDelta::FromMilliseconds(timeout_ms));
    detected_dead_event_.Reset();
    return signaled;
  }

  // AliveChecker under test.
  std::unique_ptr<AliveChecker> alive_checker_;

  // The thread the checker is run on.
  base::Thread alive_checker_thread_;

  // Mocks suspend status. Set in CreatePowerObserverHelper, owned by
  // |alive_checker_|.
  MockPowerObserverHelper* mock_power_observer_helper_;

 private:
  // The test task environment.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Event to signal dead detection.
  base::WaitableEvent detected_dead_event_;

  DISALLOW_COPY_AND_ASSIGN(AliveCheckerTest);
};

// Start and Stop, expecting no detection.
TEST_F(AliveCheckerTest, StartStop) {
  alive_checker_ = std::make_unique<AliveChecker>(
      alive_checker_thread_.task_runner(),
      base::BindRepeating(&AliveCheckerTest::DetectedDead,
                          base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
      base::TimeDelta::FromMilliseconds(kTimeoutMs), false, false,
      AliveChecker::CreatePowerObserverHelperCallback());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindRepeating(&AliveChecker::Start,
                                     base::Unretained(alive_checker_.get())));

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindRepeating(&AliveChecker::Stop,
                                     base::Unretained(alive_checker_.get())));

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(
      WaitUntilDetectedDeadWithTimeout(kTimeoutMs + kCheckIntervalMs + 10));
}

// No alive notifications, run until detection. Repeat once.
TEST_F(AliveCheckerTest, NoAliveNotificationsDetectTwice) {
  alive_checker_ = std::make_unique<AliveChecker>(
      alive_checker_thread_.task_runner(),
      base::BindRepeating(&AliveCheckerTest::DetectedDead,
                          base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
      base::TimeDelta::FromMilliseconds(kTimeoutMs), false, false,
      AliveChecker::CreatePowerObserverHelperCallback());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindRepeating(&AliveChecker::Start,
                                     base::Unretained(alive_checker_.get())));

  WaitUntilDetectedDead();

  // Verify that we don't detect a second time.
  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(
      WaitUntilDetectedDeadWithTimeout(kTimeoutMs + kCheckIntervalMs + 10));

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindRepeating(&AliveChecker::Start,
                                     base::Unretained(alive_checker_.get())));

  WaitUntilDetectedDead();
}

// Notify several times, stop, expecting no detection.
TEST_F(AliveCheckerTest, NotifyThenStop) {
  alive_checker_ = std::make_unique<AliveChecker>(
      alive_checker_thread_.task_runner(),
      base::Bind(&AliveCheckerTest::DetectedDead, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
      base::TimeDelta::FromMilliseconds(kTimeoutMs), false, false,
      AliveChecker::CreatePowerObserverHelperCallback());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AliveChecker::Start, base::Unretained(alive_checker_.get())));

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AliveChecker::Stop, base::Unretained(alive_checker_.get())));

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(
      WaitUntilDetectedDeadWithTimeout(kTimeoutMs + kCheckIntervalMs + 10));
}

// Notify several times, run until detection. Repeat once.
TEST_F(AliveCheckerTest, NotifyThenDetectDead) {
  alive_checker_ = std::make_unique<AliveChecker>(
      alive_checker_thread_.task_runner(),
      base::Bind(&AliveCheckerTest::DetectedDead, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
      base::TimeDelta::FromMilliseconds(kTimeoutMs), false, false,
      AliveChecker::CreatePowerObserverHelperCallback());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AliveChecker::Start, base::Unretained(alive_checker_.get())));

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));

  WaitUntilDetectedDead();

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AliveChecker::Start, base::Unretained(alive_checker_.get())));

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));

  WaitUntilDetectedDead();
}

// Setup to stop at first alive notification.
TEST_F(AliveCheckerTest, StopAtFirstAliveNotification) {
  alive_checker_ = std::make_unique<AliveChecker>(
      alive_checker_thread_.task_runner(),
      base::Bind(&AliveCheckerTest::DetectedDead, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
      base::TimeDelta::FromMilliseconds(kTimeoutMs), true, false,
      AliveChecker::CreatePowerObserverHelperCallback());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AliveChecker::Start, base::Unretained(alive_checker_.get())));

  alive_checker_->NotifyAlive();

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(
      WaitUntilDetectedDeadWithTimeout(kTimeoutMs + kCheckIntervalMs + 10));
}

// Notify several times, suspend and resume, run until detection.
TEST_F(AliveCheckerTest, SuspendResumeDetectDead) {
  alive_checker_ = std::make_unique<AliveChecker>(
      alive_checker_thread_.task_runner(),
      base::Bind(&AliveCheckerTest::DetectedDead, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
      base::TimeDelta::FromMilliseconds(kTimeoutMs), false, true,
      base::Bind(&AliveCheckerTest::CreatePowerObserverHelper,
                 base::Unretained(this)));
  ASSERT_TRUE(mock_power_observer_helper_);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&AliveChecker::Start, base::Unretained(alive_checker_.get())));

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&MockPowerObserverHelper::Suspend,
                            base::Unretained(mock_power_observer_helper_)));

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(
      WaitUntilDetectedDeadWithTimeout(kTimeoutMs + kCheckIntervalMs + 10));

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&MockPowerObserverHelper::Resume,
                            base::Unretained(mock_power_observer_helper_)));

  WaitUntilDetectedDead();
}

}  // namespace media
