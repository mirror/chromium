// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/message_center/message_center_controller.h"

#include "ui/message_center/message_center.h"

namespace ash {

MessageCenterController::MessageCenterController()
    : fullscreen_notification_blocker_(message_center::MessageCenter::Get()),
      inactive_user_notification_blocker_(message_center::MessageCenter::Get()),
      login_notification_blocker_(message_center::MessageCenter::Get()) {}

MessageCenterController::~MessageCenterController() {}

void MessageCenterController::BindRequest(
    mojom::AshMessageCenterControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MessageCenterController::SetClient(
    mojom::AshMessageCenterClientAssociatedPtrInfo client) {
  client_.Bind(std::move(client));
}

void MessageCenterController::ShowNotification(
    const message_center::Notification& notification) {
  auto message_center_notification =
      std::make_unique<message_center::Notification>(notification);
  // FIXME add in delegate
  message_center::MessageCenter::Get()->AddNotification(
      std::move(message_center_notification));
}

}  // namespace ash
