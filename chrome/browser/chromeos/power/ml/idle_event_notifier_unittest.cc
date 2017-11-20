// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"

namespace chromeos {
namespace power {
namespace ml {

namespace {

// Use a shorter idle delay for testing.
static constexpr int kTestIdleDelaySec = 2;
// A timeout delay so that the TestObserver won't hang waiting for an idle event
// to arrive.
static constexpr int kTestTimeoutDelaySec = 4;

class TestObserver : public IdleEventNotifier::Observer {
 public:
  TestObserver()
      : idle_event_count_(0), last_activity_time_(base::TimeTicks()) {}
  ~TestObserver() override {}

  int GetIdleEventCount() const { return idle_event_count_; }
  base::TimeTicks GetLastActivityTime() const { return last_activity_time_; }

  // IdleEventNotifier::Observer overrides:
  void OnIdleEventObserved(
      const IdleEventNotifier::ActivityData& activity_data) override {
    ++idle_event_count_;
    last_activity_time_ = activity_data.last_activity_time;
    quit_closure_.Run();
  }

  // Quit if timeout_timer_ expires and we still haven't received an idle event.
  void OnTimeout() { quit_closure_.Run(); }

  void SetQuitClosure(const base::Closure& quit_closure) {
    quit_closure_ = quit_closure;
  }

  void StartTimeoutTimer() {
    timeout_timer_.Start(FROM_HERE,
                         base::TimeDelta::FromSeconds(kTestTimeoutDelaySec),
                         this, &TestObserver::OnTimeout);
  }

 private:
  int idle_event_count_;
  base::TimeTicks last_activity_time_;
  base::Closure quit_closure_;
  // If the observer receives no idle event, we need to time out after some
  // time.
  base::OneShotTimer timeout_timer_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};
}  // namespace

class IdleEventNotifierTest : public testing::Test {
 public:
  IdleEventNotifierTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}
  ~IdleEventNotifierTest() override {}

  void SetUp() override {
    chromeos::DBusThreadManager::Initialize();
    test_observer_.reset(new TestObserver);
    idle_event_notifier_.reset(
        new IdleEventNotifier(test_observer_.get(), kTestIdleDelaySec));
    ac_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_AC);
    disconnected_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_DISCONNECTED);
  }

  void TearDown() override {
    idle_event_notifier_.reset();
    test_observer_.reset();
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<IdleEventNotifier> idle_event_notifier_;
  std::unique_ptr<TestObserver> test_observer_;
  power_manager::PowerSupplyProperties ac_power_;
  power_manager::PowerSupplyProperties disconnected_power_;

  // Called after |idle_event_notifier_| has received a signal from one of its
  // implemented interfaces. |test_observer_| will start listening for an idle
  // event from ||idle_event_notifier_| and then will check results. However,
  // |idle_event_notifier_| may not fire an idle event, so we need to set up a
  // timer so that the |test_observer_| will quit from listening after the timer
  // expires. The |timeout_timer_| is set up to have a longer timeout period
  // than the idle delay of the |idle_event_notifier_|.
  void WaitAndCheckIdleCount(int expected_idle_count,
                             base::TimeTicks* expected_last_activity_time) {
    base::RunLoop run_loop;
    test_observer_->SetQuitClosure(run_loop.QuitClosure());
    test_observer_->StartTimeoutTimer();
    run_loop.Run();
    EXPECT_EQ(expected_idle_count, test_observer_->GetIdleEventCount());
    // Optionally check |last_activity_time| is input isn't a nullptr.
    if (expected_last_activity_time != nullptr) {
      EXPECT_EQ(*expected_last_activity_time,
                test_observer_->GetLastActivityTime());
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifierTest);
};

// After initialization, |idle_delay_timer_| should not be running and
// |external_power_| is not set up.
TEST_F(IdleEventNotifierTest, CheckInitialValues) {
  EXPECT_FALSE(idle_event_notifier_->idle_delay_timer_.IsRunning());
  EXPECT_EQ(kTestIdleDelaySec, idle_event_notifier_->GetIdleDelaySec());
  EXPECT_FALSE(idle_event_notifier_->external_power_);
}

// After lid is opened, an idle event will be fired following an idle period.
TEST_F(IdleEventNotifierTest, LidOpenEventReceived) {
  base::TimeTicks now = base::TimeTicks::Now();
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN, now);
  WaitAndCheckIdleCount(1, &now);
}

// Lid-closed event will not trigger any future idle event to be generated.
TEST_F(IdleEventNotifierTest, LidClosedEventReceived) {
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::CLOSED, base::TimeTicks::Now());
  WaitAndCheckIdleCount(0, nullptr);
}

// Initially power source is unset, hence the 1st time power change signal is
// detected, it'll be treated as a user activity and an idle event will be fired
// following an idle period.
TEST_F(IdleEventNotifierTest, PowerChangedFirstSet) {
  idle_event_notifier_->PowerChanged(ac_power_);
  WaitAndCheckIdleCount(1, nullptr);
}

// PowerChanged signal is received but source isn't changed, so it won't trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceNotChanged) {
  idle_event_notifier_->PowerChanged(ac_power_);
  WaitAndCheckIdleCount(1, nullptr);
  idle_event_notifier_->PowerChanged(ac_power_);
  WaitAndCheckIdleCount(1, nullptr);
}

// PowerChanged signal is received and source is changed, so it will trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceChanged) {
  idle_event_notifier_->PowerChanged(ac_power_);
  WaitAndCheckIdleCount(1, nullptr);
  idle_event_notifier_->PowerChanged(disconnected_power_);
  WaitAndCheckIdleCount(2, nullptr);
}

// SuspendImminent will not trigger any future idle event.
TEST_F(IdleEventNotifierTest, SuspendImminent) {
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN, base::TimeTicks::Now());
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);
  WaitAndCheckIdleCount(0, nullptr);
}

// SuspendDone means user is back to active, hence it will trigger a future idle
// event.
TEST_F(IdleEventNotifierTest, SuspendDone) {
  idle_event_notifier_->SuspendDone(base::TimeDelta::FromSeconds(1));
  WaitAndCheckIdleCount(1, nullptr);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
