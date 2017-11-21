// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

namespace {

// TODO(jiameng): we will add more fields to ActivityData, hence we define an
// equality comparison below for tests.
bool operator==(const IdleEventNotifier::ActivityData& x,
                const IdleEventNotifier::ActivityData& y) {
  return x.last_activity_time == y.last_activity_time;
}

class TestObserver : public IdleEventNotifier::Observer {
 public:
  TestObserver() : idle_event_count_(0) {}
  ~TestObserver() override {}

  int idle_event_count() const { return idle_event_count_; }
  const IdleEventNotifier::ActivityData& activity_data() const {
    return activity_data_;
  }

  // IdleEventNotifier::Observer overrides:
  void OnIdleEventObserved(
      const IdleEventNotifier::ActivityData& activity_data) override {
    ++idle_event_count_;
    activity_data_ = activity_data;
  }

 private:
  int idle_event_count_;
  IdleEventNotifier::ActivityData activity_data_;
  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};
}  // namespace

class IdleEventNotifierTest : public testing::Test {
 public:
  IdleEventNotifierTest()
      : task_runner_(base::MakeRefCounted<base::TestMockTimeTaskRunner>(
            base::TestMockTimeTaskRunner::Type::kBoundToThread)),
        scoped_context_(task_runner_.get()),
        test_tick_clock_(new base::SimpleTestTickClock()) {
    test_tick_clock_->SetNowTicks(base::TimeTicks::Now());

    chromeos::DBusThreadManager::Initialize();
    idle_event_notifier_ =
        std::make_unique<IdleEventNotifier>(base::TimeDelta::FromSeconds(1));
    idle_event_notifier_->SetClockForTesting(
        std::unique_ptr<base::SimpleTestTickClock>(test_tick_clock_));
    idle_event_notifier_->AddObserver(&test_observer_);
    ac_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_AC);
    disconnected_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_DISCONNECTED);
  }

  ~IdleEventNotifierTest() override {
    // Destroy idle event notifier before DBusThreadManager is shut down.
    idle_event_notifier_.reset();
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  const scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  const base::TestMockTimeTaskRunner::ScopedContext scoped_context_;

  base::SimpleTestTickClock* test_tick_clock_;

  TestObserver test_observer_;
  std::unique_ptr<IdleEventNotifier> idle_event_notifier_;
  power_manager::PowerSupplyProperties ac_power_;
  power_manager::PowerSupplyProperties disconnected_power_;

  void fast_forward_and_check_results(
      int expected_idle_count,
      const IdleEventNotifier::ActivityData& expected_activity_data) {
    task_runner_->FastForwardUntilNoTasksRemain();
    EXPECT_EQ(expected_idle_count, test_observer_.idle_event_count());
    EXPECT_TRUE(expected_activity_data == test_observer_.activity_data());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifierTest);
};

// After initialization, |idle_delay_timer_| should not be running and
// |external_power_| is not set up.
TEST_F(IdleEventNotifierTest, CheckInitialValues) {
  EXPECT_FALSE(idle_event_notifier_->idle_delay_timer_.IsRunning());
  EXPECT_FALSE(idle_event_notifier_->external_power_);
}

// After lid is opened, an idle event will be fired following an idle period.
TEST_F(IdleEventNotifierTest, LidOpenEventReceived) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN, now);
  IdleEventNotifier::ActivityData expected_activity_data;
  expected_activity_data.last_activity_time = now;
  fast_forward_and_check_results(1, expected_activity_data);
}

// Lid-closed event will not trigger any future idle event to be generated.
TEST_F(IdleEventNotifierTest, LidClosedEventReceived) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::CLOSED, now);
  IdleEventNotifier::ActivityData expected_activity_data;
  fast_forward_and_check_results(0, expected_activity_data);
}

// Initially power source is unset, hence the 1st time power change signal is
// detected, it'll be treated as a user activity and an idle event will be fired
// following an idle period.
TEST_F(IdleEventNotifierTest, PowerChangedFirstSet) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData expected_activity_data;
  expected_activity_data.last_activity_time = now;
  fast_forward_and_check_results(1, expected_activity_data);
}

// PowerChanged signal is received but source isn't changed, so it won't trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceNotChanged) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData expected_activity_data;
  expected_activity_data.last_activity_time = now;
  fast_forward_and_check_results(1, expected_activity_data);
  idle_event_notifier_->PowerChanged(ac_power_);
  fast_forward_and_check_results(1, expected_activity_data);
}

// PowerChanged signal is received and source is changed, so it will trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceChanged) {
  base::TimeTicks now_1 = test_tick_clock_->NowTicks();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData expected_activity_data_1;
  expected_activity_data_1.last_activity_time = now_1;
  fast_forward_and_check_results(1, expected_activity_data_1);
  test_tick_clock_->Advance(base::TimeDelta::FromSeconds(100));
  base::TimeTicks now_2 = test_tick_clock_->NowTicks();
  CHECK_NE(now_1, now_2);
  idle_event_notifier_->PowerChanged(disconnected_power_);
  IdleEventNotifier::ActivityData expected_activity_data_2;
  expected_activity_data_2.last_activity_time = now_2;
  fast_forward_and_check_results(2, expected_activity_data_2);
}

// SuspendImminent will not trigger any future idle event.
TEST_F(IdleEventNotifierTest, SuspendImminent) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN, now);
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);
  IdleEventNotifier::ActivityData expected_data;
  fast_forward_and_check_results(0, expected_data);
}

// SuspendDone means user is back to active, hence it will trigger a future idle
// event.
TEST_F(IdleEventNotifierTest, SuspendDone) {
  base::TimeTicks now = test_tick_clock_->NowTicks();
  idle_event_notifier_->SuspendDone(base::TimeDelta::FromSeconds(1));
  IdleEventNotifier::ActivityData expected_activity_data;
  expected_activity_data.last_activity_time = now;
  fast_forward_and_check_results(1, expected_activity_data);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
