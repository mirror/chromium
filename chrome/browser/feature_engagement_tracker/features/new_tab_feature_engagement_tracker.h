// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_

#include "base/time/time.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

using metrics::DesktopSessionDurationTracker;
using feature_engagement_tracker::FeatureEngagementTracker;

namespace new_tab_feature_engagement_tracker {

// The NewTabFeatureEngagementTracker provides a backend for displaying
// in-product help for the New tab button.
class NewTabFeatureEngagementTracker
    : public DesktopSessionDurationTracker::Observer,
      public KeyedService {
 public:
  explicit NewTabFeatureEngagementTracker(Profile* profile);
  ~NewTabFeatureEngagementTracker() = default;

  // Notifies the feature engagement tracker that a new tab was opened.
  virtual void NotifyNewTabOpened(Profile* profile);
  // Notifies the feature engagement tracker that the omnibox has been used.
  virtual void NotifyOmniboxNavigation(Profile* profile);
  // Notifies the feature engagement tracker that the session time is up.
  virtual void NotifySessionTimeMet(Profile* profile);

  Profile* profile_;

 protected:
  virtual void TryShowPromo(FeatureEngagementTracker* feature_tracker);
  virtual bool HasEnoughSessionTimeElapsed();
  void UpdateSessionTime(base::TimeDelta elapsed, Profile* profile);

  // metrics::DesktopSessionDurationtracker::Observer::
  void OnSessionEnded(base::TimeDelta delta) override;

 private:
  int elapsed_session_time_;

  // This pointer is owned by NewTabFeatureEngagementTracker.
  DesktopSessionDurationTracker* duration_tracker_;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTracker);
};

}  // namespace new_tab_feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
