// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_WIN_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_WIN_H_

#include <windows.ui.notifications.h>
#include <string>

#include "base/macros.h"
#include "chrome/browser/notifications/notification_platform_bridge.h"

namespace winui = ABI::Windows::UI;

// Implementation of the NotificationPlatformBridge for Windows 10 Anniversary
// Edition and beyond, delegating display of notifications to the Action Center.
class NotificationPlatformBridgeWin : public NotificationPlatformBridge {
 public:
  // Handler for Notification events from Windows. Special care is needed since
  // callback may occur at any time, including at shut down! To handle this:
  // - A static singleton NotificationPlatformBridgeWin::|toast_event_handler_|
  //   is used throughout. As a result, constructor must be trivial.
  // - Operations related to NotificationPlatformBridgeWin need to be posted on
  //   UI thread.
  class ToastEventHandler {
   public:
    enum EventType {
      EVENT_TYPE_ACTIVATED,
      EVENT_TYPE_DISMISSED,
      EVENT_TYPE_FAILED,
    };

    HRESULT OnActivated(winui::Notifications::IToastNotification* notification,
                        IInspectable* inspectable);

    HRESULT OnDismissed(winui::Notifications::IToastNotification* notification,
                        winui::Notifications::IToastDismissedEventArgs* args);

    HRESULT OnFailed(winui::Notifications::IToastNotification* notification,
                     winui::Notifications::IToastFailedEventArgs* args);

   private:
    friend NotificationPlatformBridgeWin;

    // Constructor must be trivial.
    ToastEventHandler() = default;

    static void PostHandlerOnUIThread(
        EventType type,
        winui::Notifications::IToastNotification* notification);

    // Runs on the UI thread.
    static void HandleOnUIThread(EventType type,
                                 const std::string& notification_id,
                                 const std::string& profile_id);
  };

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
  // Whether the required functions from combase.dll have been loaded.
  bool com_functions_initialized_;

  friend ToastEventHandler;

  // Called when toast notification gets clicked.
  void OnClickEvent(const std::string& notification_id,
                    const std::string& profile_id);

  // Called when toast notification gets closed.
  void OnCloseEvent(const std::string& notification_id,
                    const std::string& profile_id);

  // Static instance to handle Toast event callback from Windows.
  static ToastEventHandler toast_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPlatformBridgeWin);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_WIN_H_
