// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_chromeos.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

// static
NotificationPlatformBridge* NotificationPlatformBridge::Create() {
  return new NotificationPlatformBridgeChromeOs();
}

NotificationPlatformBridgeChromeOs::NotificationPlatformBridgeChromeOs()
    : binding_(this) {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(ash::mojom::kServiceName, &controller_);

  // Register this object as the client interface implementation.
  ash::mojom::AshMessageCenterClientAssociatedPtrInfo ptr_info;
  binding_.Bind(mojo::MakeRequest(&ptr_info));
  controller_->SetClient(std::move(ptr_info));
}

NotificationPlatformBridgeChromeOs::~NotificationPlatformBridgeChromeOs() {}

void NotificationPlatformBridgeChromeOs::Display(
    NotificationCommon::Type notification_type,
    const std::string& notification_id,
    const std::string& profile_id,
    bool is_incognito,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> metadata) {
  controller_->ShowNotification(notification);
}

void NotificationPlatformBridgeChromeOs::Close(
    const std::string& profile_id,
    const std::string& notification_id) {}

void NotificationPlatformBridgeChromeOs::GetDisplayed(
    const std::string& profile_id,
    bool incognito,
    const GetDisplayedNotificationsCallback& callback) const {}

void NotificationPlatformBridgeChromeOs::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  std::move(callback).Run(true);
}

void NotificationPlatformBridgeChromeOs::HandleNotificationClosed(
    const std::string& id,
    bool by_user) {
#if 0
  ProfileNotification* notification = GetProfileNotificationById(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLOSE, notification->notification_type(),
          notification->notification().origin_url().possibly_invalid_spec(), id,
          base::nullopt, base::nullopt, by_user);
#endif
}

void NotificationPlatformBridgeChromeOs::HandleNotificationClicked(
    const std::string& id) {
#if 0
  ProfileNotification* notification = GetProfileNotificationById(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLICK, notification->notification_type(),
          notification->notification().origin_url().possibly_invalid_spec(), id,
          base::nullopt, base::nullopt, base::nullopt);
#endif
}

void NotificationPlatformBridgeChromeOs::HandleNotificationButtonClicked(
    const std::string& id,
    int button_index) {
#if 0
  ProfileNotification* notification = GetProfileNotificationById(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLICK, notification->notification_type(),
          notification->notification().origin_url().possibly_invalid_spec(), id,
          button_index, base::nullopt, base::nullopt);
#endif
}
