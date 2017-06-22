// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_DRIVE_NOTIFICATION_OBSERVER_H_
#define COMPONENTS_DRIVE_DRIVE_NOTIFICATION_OBSERVER_H_

namespace drive {

// Interface for classes which need to know when to check Google Drive for
// updates.
class DriveNotificationObserver {
 public:
  // Called when a notification from Google Drive is received.
  // |team_drive_id| is empty if the notification is for the user's changelist
  // or the Team Drive ID if the notification is for a Team Drive changelist.
  virtual void OnNotificationReceived(const std::string& team_drive_id) = 0;

  // Called when XMPP-based push notification is enabled or disabled.
  virtual void OnPushNotificationEnabled(bool enabled) {}

 protected:
  virtual ~DriveNotificationObserver() {}
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_DRIVE_NOTIFICATION_OBSERVER_H_
