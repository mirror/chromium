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
    DCHECK(TaskRunnerForTesting()->RunsTasksInCurrentSequence());
    return is_suspending_;
  }

  void Suspend() {
    DCHECK(TaskRunnerForTesting()->RunsTasksInCurrentSequence());
    is_suspending_ = true;
    SuspendCallbackForTesting()->Run();
  }

  void Resume() {
    DCHECK(TaskRunnerForTesting()->RunsTasksInCurrentSequence());
    is_suspending_ = false;
    ResumeCallbackForTesting()->Run();
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

  void OnDetectedDead() {
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
  ~AliveCheckerTest() override {
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::MANUAL,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    alive_checker_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&AliveCheckerTest::ResetAliveCheckerOnAliveCheckerThread,
                       base::Unretained(this), &done));
    done.Wait();
  }

  void CreateAliveChecker(bool stop_at_first_alive_notification,
                          bool pause_check_during_suspend) {
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::MANUAL,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    alive_checker_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &AliveCheckerTest::CreateAliveCheckerOnAliveCheckerThread,
            base::Unretained(this), stop_at_first_alive_notification,
            pause_check_during_suspend, &done));
    done.Wait();
  }

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
          base::BindOnce(&AliveCheckerTest::NotifyAliveMultipleTimes,
                         base::Unretained(this), remaining_notifications - 1,
                         delay),
          delay);
    }
  }

  void WaitUntilDetectedDead() {
    detected_dead_event_.Wait();
    detected_dead_event_.Reset();
  }

  // Returns true if the dead callback (AliveCheckerTest::OnDetectedDead) is run
  // by the AliveChecker, false if timed out.
  bool WaitUntilDetectedDeadWithTimeout(base::TimeDelta timeout) {
    bool signaled = detected_dead_event_.TimedWait(timeout);
    detected_dead_event_.Reset();
    return signaled;
  }

  // Calls AliveChecker::DetectedDead() on the |alive_checker_thread_| and
  // returns the result.
  bool GetDetectedDead() {
    bool detected_dead = false;
    base::WaitableEvent done(base::WaitableEvent::ResetPolicy::MANUAL,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);
    alive_checker_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&AliveCheckerTest::GetDetectedDeadOnAliveCheckerThread,
                       base::Unretained(this), &detected_dead, &done));
    done.Wait();
    return detected_dead;
  }

  // The thread the checker is run on.
  base::Thread alive_checker_thread_;

  // AliveChecker under test.
  std::unique_ptr<AliveChecker> alive_checker_;

  // Mocks suspend status. Set in CreatePowerObserverHelper, owned by
  // |alive_checker_|.
  MockPowerObserverHelper* mock_power_observer_helper_;

 private:
  void CreateAliveCheckerOnAliveCheckerThread(
      bool stop_at_first_alive_notification,
      bool pause_check_during_suspend,
      base::WaitableEvent* done) {
    EXPECT_TRUE(alive_checker_thread_.task_runner()->BelongsToCurrentThread());

    if (pause_check_during_suspend) {
      alive_checker_ = std::make_unique<AliveChecker>(
          base::BindRepeating(&AliveCheckerTest::OnDetectedDead,
                              base::Unretained(this)),
          base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
          base::TimeDelta::FromMilliseconds(kTimeoutMs),
          stop_at_first_alive_notification,
          base::BindOnce(&AliveCheckerTest::CreatePowerObserverHelper,
                         base::Unretained(this)));
    } else {
      alive_checker_ = std::make_unique<AliveChecker>(
          base::BindRepeating(&AliveCheckerTest::OnDetectedDead,
                              base::Unretained(this)),
          base::TimeDelta::FromMilliseconds(kCheckIntervalMs),
          base::TimeDelta::FromMilliseconds(kTimeoutMs),
          stop_at_first_alive_notification, false);
    }

    done->Signal();
  }

  void GetDetectedDeadOnAliveCheckerThread(bool* detected_dead,
                                           base::WaitableEvent* done) {
    EXPECT_TRUE(alive_checker_thread_.task_runner()->BelongsToCurrentThread());
    *detected_dead = alive_checker_->DetectedDead();
    done->Signal();
  }

  void ResetAliveCheckerOnAliveCheckerThread(base::WaitableEvent* done) {
    EXPECT_TRUE(alive_checker_thread_.task_runner()->BelongsToCurrentThread());
    alive_checker_.reset();
    done->Signal();
  }

  // The test task environment.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Event to signal that we got a dead detection callback.
  base::WaitableEvent detected_dead_event_;

  DISALLOW_COPY_AND_ASSIGN(AliveCheckerTest);
};

// Start and Stop, expecting no detection.
TEST_F(AliveCheckerTest, StartStop) {
  CreateAliveChecker(false, false);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Stop,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(WaitUntilDetectedDeadWithTimeout(
      base::TimeDelta::FromMilliseconds(kTimeoutMs + kCheckIntervalMs + 10)));
  EXPECT_FALSE(GetDetectedDead());
}

// No alive notifications, run until detection. Repeat once.
TEST_F(AliveCheckerTest, NoAliveNotificationsDetectTwice) {
  CreateAliveChecker(false, false);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  WaitUntilDetectedDead();
  EXPECT_TRUE(GetDetectedDead());

  // Verify that AliveChecker doesn't detect (runs the callback) a second time.
  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this. The detect state should still be that we have detected
  // dead.
  EXPECT_FALSE(WaitUntilDetectedDeadWithTimeout(
      base::TimeDelta::FromMilliseconds(kTimeoutMs + kCheckIntervalMs + 10)));
  EXPECT_TRUE(GetDetectedDead());

  // Start again, the detect state should be reset.
  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  WaitUntilDetectedDead();
  EXPECT_TRUE(GetDetectedDead());
}

// Notify several times, stop, expecting no detection.
TEST_F(AliveCheckerTest, NotifyThenStop) {
  CreateAliveChecker(false, false);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));
  EXPECT_FALSE(GetDetectedDead());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Stop,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(WaitUntilDetectedDeadWithTimeout(
      base::TimeDelta::FromMilliseconds(kTimeoutMs + kCheckIntervalMs + 10)));
  EXPECT_FALSE(GetDetectedDead());
}

// Notify several times, run until detection. Repeat once.
TEST_F(AliveCheckerTest, NotifyThenDetectDead) {
  CreateAliveChecker(false, false);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));

  WaitUntilDetectedDead();
  EXPECT_TRUE(GetDetectedDead());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));
  EXPECT_FALSE(GetDetectedDead());

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));
  EXPECT_FALSE(GetDetectedDead());

  WaitUntilDetectedDead();
  EXPECT_TRUE(GetDetectedDead());
}

// Setup to stop at first alive notification.
TEST_F(AliveCheckerTest, StopAtFirstAliveNotification) {
  CreateAliveChecker(true, false);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));

  alive_checker_->NotifyAlive();

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(WaitUntilDetectedDeadWithTimeout(
      base::TimeDelta::FromMilliseconds(kTimeoutMs + kCheckIntervalMs + 10)));
  EXPECT_FALSE(GetDetectedDead());
}

// Notify several times, suspend and resume, run until detection.
TEST_F(AliveCheckerTest, SuspendResumeDetectDead) {
  CreateAliveChecker(false, true);
  ASSERT_TRUE(mock_power_observer_helper_);

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&AliveChecker::Start,
                                base::Unretained(alive_checker_.get())));

  NotifyAliveMultipleTimes(
      10, base::TimeDelta::FromMilliseconds(kNotifyIntervalMs));

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&MockPowerObserverHelper::Suspend,
                                base::Unretained(mock_power_observer_helper_)));

  // It can take up to the timeout + the check interval until detection. Add a
  // margin to this.
  EXPECT_FALSE(WaitUntilDetectedDeadWithTimeout(
      base::TimeDelta::FromMilliseconds(kTimeoutMs + kCheckIntervalMs + 10)));
  EXPECT_FALSE(GetDetectedDead());

  alive_checker_thread_.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&MockPowerObserverHelper::Resume,
                                base::Unretained(mock_power_observer_helper_)));

  WaitUntilDetectedDead();
  EXPECT_TRUE(GetDetectedDead());
}

}  // namespace media
