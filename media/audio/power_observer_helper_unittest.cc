// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "media/audio/power_observer_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class PowerObserverHelperTest : public testing::Test {
 public:
  PowerObserverHelperTest()
      : power_observer_helper_thread_("AliveCheckerThread"),
        suspend_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                       base::WaitableEvent::InitialState::NOT_SIGNALED),
        resume_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                      base::WaitableEvent::InitialState::NOT_SIGNALED) {
    power_observer_helper_thread_.StartAndWaitForTesting();
  }

  void OnSuspend() {
    EXPECT_TRUE(
        power_observer_helper_thread_.task_runner()->BelongsToCurrentThread());
    suspend_event_.Signal();
  }

  void OnResume() {
    EXPECT_TRUE(
        power_observer_helper_thread_.task_runner()->BelongsToCurrentThread());
    resume_event_.Signal();
  }

 protected:
  ~PowerObserverHelperTest() override {}

  void WaitUntilSuspendNotification() {
    suspend_event_.Wait();
    suspend_event_.Reset();
  }

  void WaitUntilResumeNotification() {
    resume_event_.Wait();
    resume_event_.Reset();
  }

  bool IsSuspending() {
    bool is_suspending = false;
    base::WaitableEvent did_check(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    power_observer_helper_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&PowerObserverHelperTest::CheckIfSuspending,
                       base::Unretained(this), &is_suspending, &did_check));
    did_check.Wait();
    return is_suspending;
  }

  // PowerObserverHelper under test.
  std::unique_ptr<PowerObserverHelper> power_observer_helper_;

  // The thread the helper is run on.
  base::Thread power_observer_helper_thread_;

 private:
  void CheckIfSuspending(bool* is_suspending, base::WaitableEvent* did_check) {
    EXPECT_TRUE(
        power_observer_helper_thread_.task_runner()->BelongsToCurrentThread());
    *is_suspending = power_observer_helper_->IsSuspending();
    did_check->Signal();
  }

  // The test task environment.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Events to signal a notifications.
  base::WaitableEvent suspend_event_;
  base::WaitableEvent resume_event_;

  DISALLOW_COPY_AND_ASSIGN(PowerObserverHelperTest);
};

// Suspend and resume notifications.
TEST_F(PowerObserverHelperTest, SuspendAndResumeNotifications) {
  power_observer_helper_ = std::make_unique<PowerObserverHelper>(
      power_observer_helper_thread_.task_runner(),
      base::BindRepeating(&PowerObserverHelperTest::OnSuspend,
                          base::Unretained(this)),
      base::BindRepeating(&PowerObserverHelperTest::OnResume,
                          base::Unretained(this)));
  EXPECT_FALSE(IsSuspending());

  power_observer_helper_->OnSuspend();
  WaitUntilSuspendNotification();
  EXPECT_TRUE(IsSuspending());

  power_observer_helper_->OnResume();
  WaitUntilResumeNotification();
  EXPECT_FALSE(IsSuspending());
}

}  // namespace media
