// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_display_service.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "ui/message_center/public/cpp/notification.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif

NotificationDisplayService::~NotificationDisplayService() = default;

// static
NotificationDisplayService* NotificationDisplayService::GetForProfile(
    Profile* profile) {
  return NotificationDisplayServiceFactory::GetForProfile(profile);
}

// static
base::FilePath
NotificationDisplayService::GetProfilePathForSystemNotifications() {
#if defined(OS_CHROMEOS)
  // System notifications (such as those for network state) aren't tied to a
  // particular user and can show up before any user is logged in, so fall back
  // to the signin profile, which is guaranteed to already exist.
  return chromeos::ProfileHelper::GetSigninProfileDir();
#else
  // The "system profile" is a fallback which probably hasn't been loaded yet.
  return g_browser_process->profile_manager()->GetSystemProfilePath();
#endif
}

// static
void NotificationDisplayService::DisplaySystemNotification(
    const message_center::Notification& notification) {
  g_browser_process->profile_manager()->LoadProfileByPath(
      GetProfilePathForSystemNotifications(), false,
      base::AdaptCallbackForRepeating(base::BindOnce(
          [](message_center::Notification notification, Profile* profile) {
            NotificationDisplayService::GetForProfile(profile)->Display(
                NotificationHandler::Type::TRANSIENT, notification);
          },
          notification)));
}

// static
void NotificationDisplayService::CloseSystemNotification(
    const std::string& id) {
  Profile* profile = g_browser_process->profile_manager()->GetProfileByPath(
      GetProfilePathForSystemNotifications());
  if (!profile)
    return;

  NotificationDisplayService::GetForProfile(profile)->Close(
      NotificationHandler::Type::TRANSIENT, id);
}
