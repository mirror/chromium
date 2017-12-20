// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PERMISSION_RESTRICTIONS_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PERMISSION_RESTRICTIONS_H_

#include <map>
#include <string>

#include "base/macros.h"

// Determines the restrictions that should apply to requesting notification
// permission. Each setting has a default value, but can be influenced by
// either a Finch field trial and a chrome://flags entry.
class NotificationPermissionRestrictions {
 public:
  // Parameter names for the restrictions field trial.
  static const char kMinimumSiteEngagementParam[];
  static const char kRequireUserGestureParam[];

  NotificationPermissionRestrictions();
  ~NotificationPermissionRestrictions() = default;

  // Returns the minimum Site Engagement score that a site must have collected
  // in order to request the permission.
  double minimum_site_engagement_score() const {
    return minimum_site_engagement_score_;
  }

  // Returns whether a user gesture is required in order to request permission.
  bool require_user_gesture() const { return require_user_gesture_; }

 private:
  // Initializes the restrictions based on the |params| from the field trial.
  void InitializeWithFieldTrialParams(
      const std::map<std::string, std::string>& params);

  double minimum_site_engagement_score_ = 0;
  bool require_user_gesture_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationPermissionRestrictions);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PERMISSION_RESTRICTIONS_H_
