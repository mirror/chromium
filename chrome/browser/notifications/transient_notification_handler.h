// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_TRANSIENT_NOTIFICATION_HANDLER_H_
#define CHROME_BROWSER_NOTIFICATIONS_TRANSIENT_NOTIFICATION_HANDLER_H_

#include <set>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/browser/notifications/notification_handler.h"

class NotificationDisplayService;

class TransientNotificationHandler : public NotificationHandler {
 public:
  explicit TransientNotificationHandler(NotificationDisplayService* service);
  ~TransientNotificationHandler() override;

  // NotificationHandler implementation:
  void OnShow(Profile* profile, const std::string& notification_id) override;
  void Shutdown() override;

 private:
  // The notification service. Owns us, thus weak.
  NotificationDisplayService* service_;

  // Ids of the transient notifications that are still displaying.
  std::set<std::string> transient_notifications_;

  DISALLOW_COPY_AND_ASSIGN(TransientNotificationHandler);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_TRANSIENT_NOTIFICATION_HANDLER_H_
