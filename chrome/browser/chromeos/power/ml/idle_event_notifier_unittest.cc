// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"

#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace chromeos {
namespace power {
namespace ml {

namespace {

// Use a shorter idle delay for testing.
static constexpr int kTestIdleDelaySec = 2;
// A timeout delay so that the TestObserver won't hang waiting for an idle event
// to arrive.
static constexpr int kTestTimeoutDelaySec = 4;

bool operator==(const IdleEventNotifier::ActivityData& x,
                const IdleEventNotifier::ActivityData& y) {
  return x.last_activity_time == y.last_activity_time &&
         x.last_user_activity_time == y.last_user_activity_time &&
         x.last_mouse_time == y.last_mouse_time &&
         x.last_key_time == y.last_key_time &&
         x.earliest_active_time == y.earliest_active_time;
}

class TestObserver : public IdleEventNotifier::Observer {
 public:
  TestObserver() : idle_event_count_(0) {}
  ~TestObserver() override {}

  int GetIdleEventCount() const { return idle_event_count_; }
  IdleEventNotifier::ActivityData GetActivityData() const {
    return activity_data_;
  }

  // IdleEventNotifier::Observer overrides:
  void OnIdleEventObserved(
      const IdleEventNotifier::ActivityData& activity_data) override {
    ++idle_event_count_;
    activity_data_ = activity_data;
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
  IdleEventNotifier::ActivityData activity_data_;
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
    test_tick_clock_ = new base::SimpleTestTickClock();
    test_tick_clock_->SetNowTicks(base::TimeTicks::Now());

    test_observer_.reset(new TestObserver);
    idle_event_notifier_.reset(
        new IdleEventNotifier(test_observer_.get(), kTestIdleDelaySec));
    idle_event_notifier_->SetClockForTesting(
        std::unique_ptr<base::SimpleTestTickClock>(test_tick_clock_));
    ac_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_AC);
    disconnected_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_DISCONNECTED);
  }

  void TearDown() override {
    idle_event_notifier_.reset();
    test_observer_.reset();
    test_tick_clock_ = nullptr;
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::SimpleTestTickClock* test_tick_clock_;
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
  void WaitAndCheckIdleCount(
      int expected_idle_count,
      const IdleEventNotifier::ActivityData& expected_activity_data) {
    base::RunLoop run_loop;
    test_observer_->SetQuitClosure(run_loop.QuitClosure());
    test_observer_->StartTimeoutTimer();
    run_loop.Run();
    EXPECT_EQ(expected_idle_count, test_observer_->GetIdleEventCount());
    const IdleEventNotifier::ActivityData& result_data =
        test_observer_->GetActivityData();
    EXPECT_TRUE(expected_activity_data == result_data);
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
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN, now);
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  WaitAndCheckIdleCount(1, expected_data);
}

// Lid-closed event will not trigger any future idle event to be generated.
TEST_F(IdleEventNotifierTest, LidClosedEventReceived) {
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::CLOSED,
      test_tick_clock_->NowTicks());
  IdleEventNotifier::ActivityData expected_data;
  WaitAndCheckIdleCount(0, expected_data);
}

// Initially power source is unset, hence the 1st time power change signal is
// detected, it'll be treated as a user activity and an idle event will be fired
// following an idle period.
TEST_F(IdleEventNotifierTest, PowerChangedFirstSet) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  WaitAndCheckIdleCount(1, expected_data);
}

// PowerChanged signal is received but source isn't changed, so it won't trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceNotChanged) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  WaitAndCheckIdleCount(1, expected_data);

  idle_event_notifier_->PowerChanged(ac_power_);
  WaitAndCheckIdleCount(1, expected_data);
}

// PowerChanged signal is received and source is changed, so it will trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceChanged) {
  base::TimeTicks now_1 = test_tick_clock_->NowTicks();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData expected_data_1;
  expected_data_1.last_activity_time = now_1;
  expected_data_1.last_user_activity_time = now_1;
  WaitAndCheckIdleCount(1, expected_data_1);

  test_tick_clock_->Advance(base::TimeDelta::FromSeconds(100));
  base::TimeTicks now_2 = test_tick_clock_->NowTicks();
  CHECK_NE(now_1, now_2);
  idle_event_notifier_->PowerChanged(disconnected_power_);

  IdleEventNotifier::ActivityData expected_data_2;
  expected_data_2.last_activity_time = now_2;
  expected_data_2.last_user_activity_time = now_2;
  expected_data_2.earliest_active_time = now_1;

  WaitAndCheckIdleCount(2, expected_data_2);
}

// SuspendImminent will not trigger any future idle event.
TEST_F(IdleEventNotifierTest, SuspendImminent) {
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN, base::TimeTicks::Now());
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);
  IdleEventNotifier::ActivityData expected_data;
  WaitAndCheckIdleCount(0, expected_data);
}

// SuspendDone means user is back to active, hence it will trigger a future idle
// event.
TEST_F(IdleEventNotifierTest, SuspendDone) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->SuspendDone(base::TimeDelta::FromSeconds(1));
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  WaitAndCheckIdleCount(1, expected_data);
}

TEST_F(IdleEventNotifierTest, UserActivityKey) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  idle_event_notifier_->OnUserActivity(&key_event);
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  expected_data.last_key_time = now;
  WaitAndCheckIdleCount(1, expected_data);
}

TEST_F(IdleEventNotifierTest, UserActivityMouse) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);

  idle_event_notifier_->OnUserActivity(&mouse_event);
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  expected_data.last_mouse_time = now;
  WaitAndCheckIdleCount(1, expected_data);
}

TEST_F(IdleEventNotifierTest, UserActivityOther) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  ui::GestureEvent gesture_event(0, 0, 0, base::TimeTicks(),
                                 ui::GestureEventDetails(ui::ET_GESTURE_TAP));

  idle_event_notifier_->OnUserActivity(&gesture_event);
  IdleEventNotifier::ActivityData expected_data;
  expected_data.last_activity_time = now;
  expected_data.last_user_activity_time = now;
  WaitAndCheckIdleCount(1, expected_data);
}

// TEST_F(IdleEventNotifierTest, TwoUserActivities) {
//   ui::GestureEvent gesture_event(0, 0, 0, base::TimeTicks(),
//     ui::GestureEventDetails(ui::ET_GESTURE_TAP));

//   idle_event_notifier_->OnUserActivity(&gesture_event);
//   IdleEventNotifier::ActivityData expected_data;
//   WaitAndCheckIdleCount(0, expected_data);
// }

}  // namespace ml
}  // namespace power
}  // namespace chromeos
