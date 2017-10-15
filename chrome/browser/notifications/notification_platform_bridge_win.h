// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_WIN_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_WIN_H_

#include <windows.ui.notifications.h>
#include <map>
#include <string>

#include "base/macros.h"
#include "chrome/browser/notifications/notification_platform_bridge.h"

namespace winui = ABI::Windows::UI;

struct NotificationData;

// Implementation of the NotificationPlatformBridge for Windows 10 Anniversary
// Edition and beyond, delegating display of notifications to the Action Center.
class NotificationPlatformBridgeWin : public NotificationPlatformBridge {
 public:
  NotificationPlatformBridgeWin();
  ~NotificationPlatformBridgeWin() override;

  // NotificationPlatformBridge implementation.
  void Display(NotificationCommon::Type notification_type,
               const std::string& notification_id,
               const std::string& profile_id,
               bool incognito,
               const Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata) override;
  void Close(const std::string& profile_id,
             const std::string& notification_id) override;
  void GetDisplayed(
      const std::string& profile_id,
      bool incognito,
      const GetDisplayedNotificationsCallback& callback) const override;
  void SetReadyCallback(NotificationBridgeReadyCallback callback) override;

 private:
  // Callbacks for toast events from Windows.
  HRESULT OnActivated(winui::Notifications::IToastNotification* notification,
                      IInspectable* inspectable);
  HRESULT OnDismissed(winui::Notifications::IToastNotification* notification,
                      winui::Notifications::IToastDismissedEventArgs* args);

  void ForwardNotificationOperation(std::string notification_id,
                                    NotificationCommon::Operation operation);

  // Whether the required functions from combase.dll have been loaded.
  bool com_functions_initialized_;

  // Notification related data, indexed by notification id.
  std::map<std::string, NotificationData*> notifications_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPlatformBridgeWin);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_WIN_H_
