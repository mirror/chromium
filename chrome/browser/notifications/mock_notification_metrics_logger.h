// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_MOCK_NOTIFICATION_METRICS_LOGGER_H_
#define CHROME_BROWSER_NOTIFICATIONS_MOCK_NOTIFICATION_METRICS_LOGGER_H_

#include "base/macros.h"
#include "chrome/browser/notifications/notification_metrics_logger.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "testing/gmock/include/gmock/gmock.h"

class MockNotificationMetricsLogger : public NotificationMetricsLogger {
  public:
   // Factory function to be used with NotificationMetricsLoggerFactory's
   // SetTestingFactory method, overriding the default display service.
   static std::unique_ptr<KeyedService> FactoryForTests(
      content::BrowserContext* browser_context);

   MockNotificationMetricsLogger();
   ~MockNotificationMetricsLogger() override;

   MOCK_METHOD0(LogPersistentNotificationShown, void());
};

#endif
