// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_METRICS_NOTIFICATION_METRICS_LOGGER_IMPL_H_
#define CHROME_BROWSER_NOTIFICATIONS_METRICS_NOTIFICATION_METRICS_LOGGER_IMPL_H_

#include "chrome/browser/notifications/metrics/notification_metrics_logger.h"
#include "content/public/common/persistent_notification_status.h"

// Logs when various notification-related events have occurred.
class NotificationMetricsLoggerImpl : public NotificationMetricsLogger {
 public:
  NotificationMetricsLoggerImpl();
  ~NotificationMetricsLoggerImpl() override;

  // NotificationMetricsLogger methods.
  void LogPersistentNotificationClosedByUser() override;
  void LogPersistentNotificationClosedProgrammatically() override;
  void LogPersistentNotificationActionButtonClick() override;
  void LogPersistentNotificationClick() override;
  void LogPersistentNotificationClickResult(
      content::PersistentNotificationStatus status) override;
  void LogPersistentNotificationCloseResult(
      content::PersistentNotificationStatus status) override;
  void LogPersistentNotificationClickWithoutPermission() override;
  void LogPersistentNotificationShown() override;
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_METRICS_NOTIFICATION_METRICS_LOGGER_IMPL_H_"
