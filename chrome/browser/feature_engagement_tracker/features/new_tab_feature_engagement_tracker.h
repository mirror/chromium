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

namespace new_tab_feature_engagement_tracker {

// The NewTabFeatureEngagementTracker provides a backend for displaying
// in-product help for the new tab button.
class NewTabFeatureEngagementTracker
    : public metrics::DesktopSessionDurationTracker::Observer,
      public KeyedService {
 public:
  NewTabFeatureEngagementTracker() {}
  explicit NewTabFeatureEngagementTracker(Profile* const profile);
  ~NewTabFeatureEngagementTracker() {}

  // Notifies the feature engagement tracker that a new tab was opened.
  void NotifyNewTabOpened();
  // Notifies the feature engagement tracker that the omnibox has been used.
  void NotifyOmniboxNavigation();
  // Notifies the feature engagement tracker that the session time is up.
  void NotifySessionTimeMet();

  bool ShouldShowPromo();

  virtual feature_engagement_tracker::FeatureEngagementTracker*
  GetFeatureTracker();

  virtual PrefService* GetPrefs();

  virtual void UpdateSessionTime(base::TimeDelta elapsed);

  // metrics::DesktopSessionDurationtracker::Observer::
  void OnSessionEnded(base::TimeDelta delta) override;

 protected:
  bool HasEnoughSessionTimeElapsed();
  void ShowPromo();

 private:
  Profile* profile_;

  // This pointer is owned by NewTabFeatureEngagementTracker.
  metrics::DesktopSessionDurationTracker* duration_tracker_;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTracker);
};

}  // namespace new_tab_feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
