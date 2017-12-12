// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_display_service.h"

#include "chrome/browser/notifications/notification_display_service_factory.h"

// static
NotificationDisplayService* NotificationDisplayService::GetForProfile(
    Profile* profile) {
  return NotificationDisplayServiceFactory::GetForProfile(profile);
}

#if !defined(OS_CHROMEOS)
// static
NotificationDisplayService*
NotificationDisplayService::GetForSystemNotifications() {
  if (!g_browser_process->profile_manager()->GetProfile(
          ProfileManager::GetSystemProfilePath()))
    return nullptr;

  return NotificationDisplayService::GetForProfile(
      g_browser_process->profile_manager()->GetProfile(
          ProfileManager::GetSystemProfilePath()));
}
#endif

// static
void NotificationDisplayService::DisplayForSystemNotification(
    const Notification& notification) {
  NotificationDisplayService* service = GetForSystemNotifications();
  if (service) {
    service->Display(NotificationHandler::Type::TRANSIENT, notification);
    return;
  }

  g_browser_process->profile_manager()->CreateProfileAsync(
      ProfileManager::GetSystemProfilePath(),
      base::Bind(
          [](Notification notification, Profile* profile,
             Profile::CreateStatus status) {
            NotificationDisplayService::GetForProfile(profile)->Display(
                notification);
          },
          notification),
      base::string16(), std::string(), std::string());
}

NotificationDisplayService::~NotificationDisplayService() = default;
