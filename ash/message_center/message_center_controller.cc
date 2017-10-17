// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/message_center/message_center_controller.h"

#include "ui/message_center/message_center.h"

using message_center::MessageCenter;

namespace ash {

MessageCenterController::MessageCenterController()
    : fullscreen_notification_blocker_(message_center::MessageCenter::Get()),
      inactive_user_notification_blocker_(message_center::MessageCenter::Get()),
      login_notification_blocker_(message_center::MessageCenter::Get()) {
  MessageCenter::Get()->AddObserver(this);
}

MessageCenterController::~MessageCenterController() {
  MessageCenter::Get()->RemoveObserver(this);
}

void MessageCenterController::AddNotification(
    std::unique_ptr<message_center::Notification> notification) {
  if (MessageCenter::Get()->FindVisibleNotificationById(notification.id())) {
    UpdateNotification(std::move(notification);
    return;
  }

  delegates_.insert(std::make_pair(notification->id(), notification->delegate()));
  message_center::MessageCenter::Get()->AddNotification(std::move(notification));
}

void MessageCenterController::UpdateNotification(
    std::unique_ptr<message_center::Notification> notification) {
  std::string id = notification->id();
  message_center::MessageCenter::Get()->UpdateNotification(
      id, std::move(notification));
}

void MessageCenterController::RemoveNotification(const std::string& id) {
  MessageCenter::Get()->RemoveNotification(id, false /*by_user*/);
}

void MessageCenterController::OnNotificationRemoved(const std::string& id,
                                                    bool by_user) {
  delegates_.erase(id);
}

void MessageCenterController::OnNotificationClicked(const std::string& id) {
  auto iter = delegates_.find(id);
  if (iter != delegates_.end())
    iter->second->Click();
}

void MessageCenterController::OnNotificationButtonClicked(
    const std::string& id, int button_index) {
  auto iter = delegates_.find(id);
  if (iter != delegates_.end())
    iter->second->ButtonClick(button_index);
}

}  // namespace ash
