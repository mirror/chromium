// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger.h"

#include <memory>
#include <vector>

#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"
#include "chrome/browser/chromeos/power/ml/logging_helper.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

void ExpectEqualEvent(const UserActivityEvent& result_event,
                      const UserActivityEvent::Event& expected_event) {
  EXPECT_EQ(result_event.event().SerializeAsString(),
            expected_event.SerializeAsString());
}

// Testing logging helper.
class TestingLoggingHelper : public LoggingHelper {
 public:
  TestingLoggingHelper() {}
  ~TestingLoggingHelper() override {}
  void LogActivity(const UserActivityEvent& event) override {
    events_.push_back(event);
  }

  std::vector<UserActivityEvent> GetLoggedEvents() { return events_; }

 private:
  std::vector<UserActivityEvent> events_;
};

class UserActivityLoggerTest : public testing::Test {
 public:
  UserActivityLoggerTest() : activity_logger_(nullptr) {
    activity_logger_.reset(new UserActivityLogger(&helper_));
  }

  ~UserActivityLoggerTest() override = default;

 protected:
  void SetUserEvent(const ui::Event* event) {
    activity_logger_->OnUserActivity(event);
  }

  void SetIdleEvent(const IdleEventNotifier::ActivityData& data) {
    activity_logger_->OnIdleEventObserved(data);
  }

  std::vector<UserActivityEvent> GetEvents() {
    return helper_.GetLoggedEvents();
  }

 private:
  TestingLoggingHelper helper_;
  std::unique_ptr<UserActivityLogger> activity_logger_;
};

// After an idle event, we have a ui::Event, we should expect one
// UserActivityEvent.
TEST_F(UserActivityLoggerTest, LogAfterIdleEvent) {
  // Trigger an idle event.
  IdleEventNotifier::ActivityData data;
  data.last_activity_time = base::TimeTicks::Now();
  SetIdleEvent(data);
  // Pass in nullptr since ui::Event is not used.
  SetUserEvent(nullptr);
  // Expect one event.
  const auto& events = GetEvents();
  EXPECT_EQ(events.size(), 1ul);
  // Expected event.
  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  ExpectEqualEvent(events[0], expected_event);
}

// Get a user event before an idle event, we should not log it.
TEST_F(UserActivityLoggerTest, LogBeforeIdleEvent) {
  // Pass in nullptr since ui::Event is not used.
  SetUserEvent(nullptr);
  // Trigger an idle event.
  IdleEventNotifier::ActivityData data;
  data.last_activity_time = base::TimeTicks::Now();
  SetIdleEvent(data);
  // Expect zero event.
  const auto& events = GetEvents();
  EXPECT_EQ(events.size(), 0ul);
}

// Get a user event, then an idle event, then another user event,
// we should log the last one/
TEST_F(UserActivityLoggerTest, LogSecondEvent) {
  // Trigger user event.
  SetUserEvent(nullptr);
  // Trigger an idle event.
  IdleEventNotifier::ActivityData data;
  data.last_activity_time = base::TimeTicks::Now();
  SetIdleEvent(data);
  // Another user event.
  SetUserEvent(nullptr);
  // Expect one event.
  const auto& events = GetEvents();
  EXPECT_EQ(events.size(), 1ul);
  // Expected event.
  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  ExpectEqualEvent(events[0], expected_event);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
