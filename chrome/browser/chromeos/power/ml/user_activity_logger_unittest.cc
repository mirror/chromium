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
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

// Testing logger delegate.
class TestingUserActivityLoggerDelegate : public UserActivityLoggerDelegate {
 public:
  TestingUserActivityLoggerDelegate() = default;
  ~TestingUserActivityLoggerDelegate() override {}

  const std::vector<UserActivityEvent>& GetLoggedEvents() const {
    return events_;
  }

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

  std::vector<UserActivityEvent> GetEvents() {
    return delegate_.GetLoggedEvents();
  }

 private:
  TestingUserActivityLoggerDelegate delegate_;
  UserActivityLogger activity_logger_;
};

// After an idle event, we have a ui::Event, we should expect one
// UserActivityEvent.
TEST_F(UserActivityLoggerTest, LogAfterIdleEvent) {
  // Trigger an idle event.
  IdleEventNotifier::ActivityData data;
  data.last_activity_time = base::TimeTicks::Now();
  ReportIdleEvent(data);
  ReportUserActivity(nullptr);

  const auto& events = GetEvents();
  ASSERT_EQ(events.size(), 1ul);

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EXPECT_EQ(events[0].event().SerializeAsString(),
            expected_event.SerializeAsString());
}

// Get a user event before an idle event, we should not log it.
TEST_F(UserActivityLoggerTest, LogBeforeIdleEvent) {
  ReportUserActivity(nullptr);
  // Trigger an idle event.
  IdleEventNotifier::ActivityData data;
  data.last_activity_time = base::TimeTicks::Now();
  ReportIdleEvent(data);

  EXPECT_EQ(GetEvents().size(), 0ul);
}

// Get a user event, then an idle event, then another user event,
// we should log the last one/
TEST_F(UserActivityLoggerTest, LogSecondEvent) {
  ReportUserActivity(nullptr);
  // Trigger an idle event.
  IdleEventNotifier::ActivityData data;
  data.last_activity_time = base::TimeTicks::Now();
  ReportIdleEvent(data);
  // Another user event.
  ReportUserActivity(nullptr);

  const auto& events = GetEvents();
  ASSERT_EQ(events.size(), 1ul);

  UserActivityEvent::Event expected_event;
  expected_event.set_type(UserActivityEvent::Event::REACTIVATE);
  expected_event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);
  EXPECT_EQ(events[0].event().SerializeAsString(),
            expected_event.SerializeAsString());
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
