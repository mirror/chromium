// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_CHROMEOS_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_CHROMEOS_H_

#include <map>
#include <string>

#include "ash/public/interfaces/ash_message_center_controller.mojom.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "chrome/browser/notifications/notification_platform_bridge.h"
#include "chrome/browser/notifications/profile_notification.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

// A platform bridge that uses Ash's message center to display notifications.
// Currently under development and controlled by feature:
//   --enable-features=NativeNifications
class NotificationPlatformBridgeChromeOs
    : public NotificationPlatformBridge,
      public ash::mojom::AshMessageCenterClient {
 public:
  NotificationPlatformBridgeChromeOs();

  ~NotificationPlatformBridgeChromeOs() override;

  // NotificationPlatformBridge:
  void Display(NotificationCommon::Type notification_type,
               const std::string& notification_id,
               const std::string& profile_id,
               bool is_incognito,
               const message_center::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata) override;
  void Close(const std::string& profile_id,
             const std::string& notification_id) override;
  void GetDisplayed(
      const std::string& profile_id,
      bool incognito,
      const GetDisplayedNotificationsCallback& callback) const override;
  void SetReadyCallback(NotificationBridgeReadyCallback callback) override;

  // AshMessageCenterClient:
  void HandleNotificationClosed(const std::string& id, bool by_user) override;
  void HandleNotificationClicked(const std::string& id) override;
  void HandleNotificationButtonClicked(const std::string& id,
                                       int button_index) override;

 private:
  ProfileNotification* GetProfileNotification(
      const std::string& profile_notification_id);

  // A container for all active notifications, where IDs are permuted to
  // uniquely identify both the notification and its source profile. The key is
  // the permuted ID.
  std::map<std::string, std::unique_ptr<ProfileNotification>>
      active_notifications_;

  ash::mojom::AshMessageCenterControllerPtr controller_;
  mojo::AssociatedBinding<ash::mojom::AshMessageCenterClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPlatformBridgeChromeOs);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_CHROMEOS_H_
