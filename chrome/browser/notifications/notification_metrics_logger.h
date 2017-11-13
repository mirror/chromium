// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_METRICS_LOGGER_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_METRICS_LOGGER_H_

#include "components/keyed_service/core/keyed_service.h"

// Logs when various notification-related events have occurred.
class NotificationMetricsLogger : public KeyedService {
 public:
  NotificationMetricsLogger();
  ~NotificationMetricsLogger() override;

  // Logs that a persistent notification has been displayed.
  virtual void LogPersistentNotificationShown() {}
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_METRICS_LOGGER_H_"
