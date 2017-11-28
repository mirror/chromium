// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_DISPLAY_SERVICE_IMPL_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_DISPLAY_SERVICE_IMPL_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_handler.h"

class GURL;
class Profile;

// Implementation of the NotificationDisplayService interface.
class NotificationDisplayServiceImpl : public NotificationDisplayService {
 public:
  explicit NotificationDisplayService(Profile* profile);
  ~NotificationDisplayService() override;

  // Used to propagate back events originate from the user. The events are
  // received and dispatched to the right consumer depending on the type of
  // notification. Consumers include, service workers, pages, extensions...
  //
  // TODO(peter): Remove this in favor of multiple targetted methods.
  void ProcessNotificationOperation(NotificationCommon::Operation operation,
                                    NotificationHandler::Type notification_type,
                                    const GURL& origin,
                                    const std::string& notification_id,
                                    const base::Optional<int>& action_index,
                                    const base::Optional<base::string16>& reply,
                                    const base::Optional<bool>& by_user);

  // Registers an implementation object to handle notification operations
  // for |notification_type|.
  void AddNotificationHandler(NotificationHandler::Type notification_type,
                              std::unique_ptr<NotificationHandler> handler);

  // Returns the notification handler that was registered for the given type.
  // May return null.
  NotificationHandler* GetNotificationHandler(
      NotificationHandler::Type notification_type);

  // Removes an implementation object added via AddNotificationHandler.
  void RemoveNotificationHandler(NotificationHandler::Type notification_type);

  // NotificationDisplayService implementation:
  void Display(NotificationHandler::Type notification_type,
               const message_center::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata =
                   nullptr) override;
  void Close(NotificationHandler::Type notification_type,
             const std::string& notification_id) override;
  void GetDisplayed(const DisplayedNotificationsCallback& callback) override;

 private:
  std::map<NotificationHandler::Type, std::unique_ptr<NotificationHandler>>
      notification_handlers_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NotificationDisplayServiceImpl);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_DISPLAY_SERVICE_IMPL_H_
