// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MESSAGE_CENTER_MESSAGE_CENTER_CONTROLLER_H_
#define ASH_MESSAGE_CENTER_MESSAGE_CENTER_CONTROLLER_H_

#include "ash/system/web_notification/fullscreen_notification_blocker.h"
#include "ash/system/web_notification/inactive_user_notification_blocker.h"
#include "ash/system/web_notification/login_state_notification_blocker.h"
#include "base/macros.h"
#include "ui/message_center/message_center_observer.h"

namespace ash {

// This class manages the ash message center. For now it just houses
// Ash-specific notification blockers. In the future, when the MessageCenter
// lives in the Ash process, it will also manage adding and removing
// notifications sent from clients (like Chrome).
class MessageCenterController : public message_center::MessageCenterObserver {
 public:
  MessageCenterController();
  ~MessageCenterController() override;

  // TODO(estade): eventually, Notification will be a data-only object that no
  // longer contains a NotificationDelegate. At that point, this function will
  // need a NotificationDelegate parameter.
  // FIXME also updates?
  void AddNotification(
      std::unique_ptr<message_center::Notification> notification);

  // Does nothing if a notification with matching ID is not already present.
  void UpdateNotification(
      std::unique_ptr<message_center::Notification> notification);

  // Programatically remove a notification (i.e., not due to a user action).
  void RemoveNotification(const std::string& id);

  // MessageCenterObserver:
  void OnNotificationRemoved(const std::string& id, bool by_user)override;
  void OnNotificationClicked(const std::string& id)override;
  void OnNotificationButtonClicked(const std::string& id, int button_index)override;

  InactiveUserNotificationBlocker*
  inactive_user_notification_blocker_for_testing() {
    return &inactive_user_notification_blocker_;
  }

 private:
  FullscreenNotificationBlocker fullscreen_notification_blocker_;
  InactiveUserNotificationBlocker inactive_user_notification_blocker_;
  LoginStateNotificationBlocker login_notification_blocker_;

  std::map<std::string, scoped_refptr<message_center::NotificationDelegate>>
      delegates_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenterController);
};

}  // namespace ash

#endif  // ASH_MESSAGE_CENTER_MESSAGE_CENTER_CONTROLLER_H_
