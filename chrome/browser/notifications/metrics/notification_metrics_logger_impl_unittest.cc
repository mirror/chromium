// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/metrics/notification_metrics_logger_impl.h"

#include "base/test/histogram_tester.h"
#include "base/test/user_action_tester.h"
#include "content/public/common/persistent_notification_status.h"
#include "testing/gtest/include/gtest/gtest.h"

class NotificationMetricsLoggerImplTest : public ::testing::Test {
 protected:
  base::UserActionTester user_action_tester_;
  base::HistogramTester histogram_tester_;
  NotificationMetricsLoggerImpl logger_;
};

TEST_F(NotificationMetricsLoggerImplTest, PersistentNotificationShown) {
  logger_.LogPersistentNotificationShown();
  EXPECT_EQ(
      1, user_action_tester_.GetActionCount("Notifications.Persistent.Shown"));
}

TEST_F(NotificationMetricsLoggerImplTest, PersistentNotificationClosedByUser) {
  logger_.LogPersistentNotificationClosedByUser();
  EXPECT_EQ(1, user_action_tester_.GetActionCount(
                   "Notifications.Persistent.ClosedByUser"));
}

TEST_F(NotificationMetricsLoggerImplTest,
       PersistentNotificationClosedProgrammatically) {
  logger_.LogPersistentNotificationClosedProgrammatically();
  EXPECT_EQ(1, user_action_tester_.GetActionCount(
                   "Notifications.Persistent.ClosedProgrammatically"));
}

TEST_F(NotificationMetricsLoggerImplTest, PersistentNotificationClick) {
  logger_.LogPersistentNotificationClick();
  EXPECT_EQ(1, user_action_tester_.GetActionCount(
                   "Notifications.Persistent.Clicked"));
}

TEST_F(NotificationMetricsLoggerImplTest,
       PersistentNotificationClickNoPermission) {
  logger_.LogPersistentNotificationClickWithoutPermission();
  EXPECT_EQ(1, user_action_tester_.GetActionCount(
                   "Notifications.Persistent.ClickedWithoutPermission"));
}

TEST_F(NotificationMetricsLoggerImplTest, PersistentNotificationAction) {
  logger_.LogPersistentNotificationActionButtonClick();
  EXPECT_EQ(1, user_action_tester_.GetActionCount(
                   "Notifications.Persistent.ClickedActionButton"));
}

TEST_F(NotificationMetricsLoggerImplTest, PersistentNotificationClickResult) {
  logger_.LogPersistentNotificationClickResult(
      content::PersistentNotificationStatus::
          PERSISTENT_NOTIFICATION_STATUS_SUCCESS);
  histogram_tester_.ExpectUniqueSample(
      "Notifications.PersistentWebNotificationClickResult",
      0 /* SERVICE_WORKER_OK */, 1);
}
