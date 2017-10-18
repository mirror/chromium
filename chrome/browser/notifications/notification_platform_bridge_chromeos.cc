// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_platform_bridge_chromeos.h"

#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

Profile* GetProfile(const std::string& profile_id, bool incognito) {
  ProfileManager* manager = g_browser_process->profile_manager();
  Profile* profile =
      manager->GetProfile(manager->user_data_dir().AppendASCII(profile_id));
  return incognito ? profile->GetOffTheRecordProfile() : profile;
}

}  // namespace

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
  auto active_notification = std::make_unique<ProfileNotification>(
      GetProfile(profile_id, is_incognito), notification, notification_type);
  controller_->ShowClientNotification(active_notification->notification());

  std::string profile_notification_id =
      active_notification->notification().id();
  active_notifications_.emplace(profile_notification_id,
                                std::move(active_notification));
}

void NotificationPlatformBridgeChromeOs::Close(
    const std::string& profile_id,
    const std::string& notification_id) {
  NOTIMPLEMENTED();
}

void NotificationPlatformBridgeChromeOs::GetDisplayed(
    const std::string& profile_id,
    bool incognito,
    const GetDisplayedNotificationsCallback& callback) const {
  NOTIMPLEMENTED();
}

void NotificationPlatformBridgeChromeOs::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  std::move(callback).Run(true);
}

void NotificationPlatformBridgeChromeOs::HandleNotificationClosed(
    const std::string& id,
    bool by_user) {
  ProfileNotification* notification = GetProfileNotification(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLOSE, notification->type(),
          notification->notification().origin_url().possibly_invalid_spec(),
          notification->original_id(), base::nullopt, base::nullopt, by_user);
}

void NotificationPlatformBridgeChromeOs::HandleNotificationClicked(
    const std::string& id) {
  ProfileNotification* notification = GetProfileNotification(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLICK, notification->type(),
          notification->notification().origin_url().possibly_invalid_spec(),
          notification->original_id(), base::nullopt, base::nullopt,
          base::nullopt);
}

void NotificationPlatformBridgeChromeOs::HandleNotificationButtonClicked(
    const std::string& id,
    int button_index) {
  ProfileNotification* notification = GetProfileNotification(id);
  NotificationDisplayServiceFactory::GetForProfile(notification->profile())
      ->ProcessNotificationOperation(
          NotificationCommon::CLICK, notification->type(),
          notification->notification().origin_url().possibly_invalid_spec(),
          notification->original_id(), button_index, base::nullopt,
          base::nullopt);
}

ProfileNotification* NotificationPlatformBridgeChromeOs::GetProfileNotification(
    const std::string& profile_notification_id) {
  auto iter = active_notifications_.find(profile_notification_id);
  DCHECK(iter != active_notifications_.end());
  return iter->second.get();
}
