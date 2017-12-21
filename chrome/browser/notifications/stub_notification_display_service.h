// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_STUB_NOTIFICATION_DISPLAY_SERVICE_H_
#define CHROME_BROWSER_NOTIFICATIONS_STUB_NOTIFICATION_DISPLAY_SERVICE_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "ui/message_center/notification.h"

class Profile;

// Implementation of the NotificationDisplayService interface that can be used
// for testing purposes. Supports additional methods enabling instrumenting the
// faked underlying notification system.
class StubNotificationDisplayService : public NotificationDisplayService {
 public:
  // Factory function to be used with NotificationDisplayServiceFactory's
  // SetTestingFactory method, overriding the default display service.
  static std::unique_ptr<KeyedService> FactoryForTests(
      content::BrowserContext* browser_context);

  explicit StubNotificationDisplayService(Profile* profile);
  ~StubNotificationDisplayService() override;

  // Sets |closure| to be invoked when any notification has been added.
  void SetNotificationAddedClosure(base::RepeatingClosure closure);

  // Returns a vector of the displayed Notification objects.
  std::vector<message_center::Notification> GetDisplayedNotificationsForType(
      NotificationCommon::Type type) const;

  base::Optional<message_center::Notification> GetNotification(
      const std::string& notification_id);

  const NotificationCommon::Metadata* GetMetadataForNotification(
      const message_center::Notification& notification);

  // Simulates the notification identified by |notification_id| being closed due
  // to external events, such as the user dismissing it when |by_user| is set.
  // Will wait for the close event to complete. When |silent| is set, the
  // notification handlers won't be informed of the change to immitate behaviour
  // of operating systems that don't inform apps about removed notifications.
  void RemoveNotification(NotificationCommon::Type notification_type,
                          const std::string& notification_id,
                          bool by_user,
                          bool silent);

  // Removes all notifications shown by this display service. Will wait for the
  // close events to complete.
  void RemoveAllNotifications(NotificationCommon::Type notification_type,
                              bool by_user);

  NotificationCommon::Operation get_last_operation() { return last_operation_; }
  NotificationCommon::Type get_last_notification_type() {
    return last_notification_type_;
  }
  GURL get_last_origin() { return last_origin_; }
  std::string get_last_notification_id() { return last_notification_id_; }
  base::Optional<int> get_last_action_index() { return last_action_index_; }
  base::Optional<base::string16> get_last_reply() { return last_reply_; }
  base::Optional<bool> get_last_by_user() { return last_by_user_; }

  // NotificationDisplayService implementation:
  void Display(NotificationCommon::Type notification_type,
               const message_center::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata) override;
  void Close(NotificationCommon::Type notification_type,
             const std::string& notification_id) override;
  void GetDisplayed(const DisplayedNotificationsCallback& callback) override;
  void ProcessNotificationOperation(
      NotificationCommon::Operation operation,
      NotificationCommon::Type notification_type,
      const GURL& origin,
      const std::string& notification_id,
      const base::Optional<int>& action_index,
      const base::Optional<base::string16>& reply,
      const base::Optional<bool>& by_user) override;

 private:
  // Data to store for a notification that's being shown through this service.
  struct NotificationData {
    NotificationData(NotificationCommon::Type type,
                     const message_center::Notification& notification,
                     std::unique_ptr<NotificationCommon::Metadata> metadata);
    NotificationData(NotificationData&& other);
    ~NotificationData();

    NotificationData& operator=(NotificationData&& other);

    NotificationCommon::Type type;
    message_center::Notification notification;
    std::unique_ptr<NotificationCommon::Metadata> metadata;
  };

  base::RepeatingClosure notification_added_closure_;
  std::vector<NotificationData> notifications_;
  Profile* profile_;

  NotificationCommon::Operation last_operation_;
  NotificationCommon::Type last_notification_type_;
  GURL last_origin_;
  std::string last_notification_id_;
  base::Optional<int> last_action_index_;
  base::Optional<base::string16> last_reply_;
  base::Optional<bool> last_by_user_;

  DISALLOW_COPY_AND_ASSIGN(StubNotificationDisplayService);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_STUB_NOTIFICATION_DISPLAY_SERVICE_H_
