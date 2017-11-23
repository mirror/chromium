// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger.h"

#include <memory>
#include <vector>

#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

void EqualEvent(const UserActivityEvent::Event& expected_event,
                const UserActivityEvent::Event& result_event) {
  EXPECT_EQ(expected_event.type(), result_event.type());
  EXPECT_EQ(expected_event.reason(), result_event.reason());
}

void EqualFeatures(const UserActivityEvent::Features& expected_features,
                   const UserActivityEvent::Features& result_features) {
  EXPECT_EQ(expected_features.device_mode(), result_features.device_mode());
  EXPECT_EQ(expected_features.battery_percent(),
            result_features.battery_percent());
  EXPECT_EQ(expected_features.on_battery(), result_features.on_battery());
}

// Testing logger delegate.
class TestingUserActivityLoggerDelegate : public UserActivityLoggerDelegate {
 public:
  TestingUserActivityLoggerDelegate() = default;
  ~TestingUserActivityLoggerDelegate() override = default;

  const std::vector<UserActivityEvent>& events() const { return events_; }

  void LogActivity(const UserActivityEvent& event) override {
    events_.push_back(event);
  }

 private:
  std::vector<UserActivityEvent> events_;

  DISALLOW_COPY_AND_ASSIGN(TestingUserActivityLoggerDelegate);
};

class UserActivityLoggerTest : public testing::Test {
 public:
  UserActivityLoggerTest() : activity_logger_(&delegate_) {}

  ~UserActivityLoggerTest() override = default;

 protected:
  void ReportUserActivity(const ui::Event* event) {
    activity_logger_.OnUserActivity(event);
  }

  void ReportIdleEvent(const IdleEventNotifier::ActivityData& data) {
    activity_logger_.OnIdleEventObserved(data);
  }

  void ReportLidEvent(chromeos::PowerManagerClient::LidState state) {
    activity_logger_.LidEventReceived(state, base::TimeTicks::Now());
  }

  void ReportPowerChangeEvent(
      power_manager::PowerSupplyProperties::ExternalPower power,
      float battery_percent) {
    power_manager::PowerSupplyProperties proto;
    proto.set_external_power(power);
    proto.set_battery_percent(battery_percent);
    activity_logger_.PowerChanged(proto);
  }

  void ReportTabletModeEvent(chromeos::PowerManagerClient::TabletMode mode) {
    activity_logger_.TabletModeEventReceived(mode, base::TimeTicks::Now());
  }

  const std::vector<UserActivityEvent>& GetEvents() {
    return delegate_.events();
  }

 private:
  TestingUserActivityLoggerDelegate delegate_;
  UserActivityLogger activity_logger_;
};

// After an idle event, we have a ui::Event, we should expect one
// UserActivityEvent.
TEST_F(UserActivityLoggerTest, LogAfterIdleEvent) {
  // Trigger an idle event.
  ReportIdleEvent({base::Time::Now()});
  ReportUserActivity(nullptr);

  const auto& events = GetEvents();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EqualEvent(expected_event, events[0].event());
}

// Get a user event before an idle event, we should not log it.
TEST_F(UserActivityLoggerTest, LogBeforeIdleEvent) {
  ReportUserActivity(nullptr);
  // Trigger an idle event.
  ReportIdleEvent({base::Time::Now()});

  EXPECT_EQ(0U, GetEvents().size());
}

// Get a user event, then an idle event, then another user event,
// we should log the last one.
TEST_F(UserActivityLoggerTest, LogSecondEvent) {
  ReportUserActivity(nullptr);
  // Trigger an idle event.
  ReportIdleEvent({base::Time::Now()});
  // Another user event.
  ReportUserActivity(nullptr);

  const auto& events = GetEvents();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EqualEvent(expected_event, events[0].event());
}

// Log multiple events.
TEST_F(UserActivityLoggerTest, LogMultipleEvents) {
  // Trigger an idle event.
  ReportIdleEvent({base::Time::Now()});
  // First user event.
  ReportUserActivity(nullptr);

  // Trigger an idle event.
  ReportIdleEvent({base::Time::Now()});
  // Second user event.
  ReportUserActivity(nullptr);

  const auto& events = GetEvents();
  ASSERT_EQ(2U, events.size());

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EqualEvent(expected_event, events[0].event());
  EqualEvent(expected_event, events[1].event());
}

// Test feature extraction.
TEST_F(UserActivityLoggerTest, FeatureExtraction) {
  ReportIdleEvent({base::Time::Now()});

  ReportLidEvent(chromeos::PowerManagerClient::LidState::OPEN);
  ReportTabletModeEvent(chromeos::PowerManagerClient::TabletMode::UNSUPPORTED);
  ReportPowerChangeEvent(power_manager::PowerSupplyProperties::AC, 23.0f);

  ReportUserActivity(nullptr);

  const auto& events = GetEvents();
  ASSERT_EQ(1U, events.size());

  UserActivityEvent::Features expected_features;
  expected_features.set_device_mode(UserActivityEvent::Features::CLAMSHELL);
  expected_features.set_on_battery(false);
  expected_features.set_battery_percent(23.0f);
  EqualFeatures(expected_features, events[0].features());
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
