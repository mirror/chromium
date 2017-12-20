// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_permission_restrictions.h"

#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/chrome_features.h"

// static
const char NotificationPermissionRestrictions::kMinimumSiteEngagementParam[] =
    "minimum_engagement";
const char NotificationPermissionRestrictions::kRequireUserGestureParam[] =
    "user_gesture";

NotificationPermissionRestrictions::NotificationPermissionRestrictions() {
  std::map<std::string, std::string> params;
  if (base::GetFieldTrialParamsByFeature(
          features::kNotificationPermissionRestrictions, &params)) {
    InitializeWithFieldTrialParams(params);
  }
}

void NotificationPermissionRestrictions::InitializeWithFieldTrialParams(
    const std::map<std::string, std::string>& params) {
  auto minimum_site_engagement_score_iter =
      params.find(kMinimumSiteEngagementParam);
  if (minimum_site_engagement_score_iter != params.end()) {
    double minimum_site_engagement_score = 0;
    if (base::StringToDouble(minimum_site_engagement_score_iter->second,
                             &minimum_site_engagement_score)) {
      minimum_site_engagement_score_ = minimum_site_engagement_score;
    }
  }

  auto require_user_gesture_iter = params.find(kRequireUserGestureParam);
  if (require_user_gesture_iter != params.end()) {
    require_user_gesture_ = require_user_gesture_iter->second == "true";
  }
}
