// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_COMMON_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_COMMON_H_

#include "base/feature_list.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "url/gurl.h"

class GURL;
class Profile;

// Shared functionality for both in page and persistent notification
class NotificationCommon {
 public:
  // Things as user can do to a notification.
  // TODO(peter): Prefix these options with OPERATION_.
  enum Operation {
    CLICK = 0,
    CLOSE = 1,
    DISABLE_PERMISSION = 2,
    SETTINGS = 3,
    OPERATION_MAX = SETTINGS
  };

  // A struct that contains extra data about a notification specific to one of
  // the types in NotificationHandler.
  struct Metadata {
    virtual ~Metadata();

    NotificationHandler::Type type;
  };

  // Open the Notification settings screen when clicking the right button.
  static void OpenNotificationSettings(Profile* profile, const GURL& origin);
};

// Metadata specific to web notifications (persistent and non).
struct WebNotificationMetadata : public NotificationCommon::Metadata {
  WebNotificationMetadata();
  ~WebNotificationMetadata() override;

  static const WebNotificationMetadata* From(const Metadata* metadata);

  // Vibration pattern to play when displaying the notification. There must be
  // an odd number of entries in this pattern when it's set: numbers of
  // milliseconds to vibrate separated by numbers of milliseconds to pause.
  // Currently only respected on Android.
  std::vector<int> vibration_pattern;

  // Whether the vibration pattern and other applicable announcement mechanisms
  // should be considered when updating the notification.
  bool renotify = false;

  // When true, the notification should not make a noise or vibration regardless
  // of device settings.
  bool silent = false;
};

// Metadata for WEB_PERSISTENT notifications.
struct PersistentWebNotificationMetadata : public WebNotificationMetadata {
  PersistentWebNotificationMetadata();
  ~PersistentWebNotificationMetadata() override;

  static const PersistentWebNotificationMetadata* From(
      const Metadata* metadata);

  GURL service_worker_scope;
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_COMMON_H_
