// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_

#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"

using metrics::DesktopSessionDurationTracker;

namespace feature_engagement_tracker {

class NewTabFeatureEngagementTracker
    : public DesktopSessionDurationTracker::Observer {
 public:
  static void Initialize();
  static bool IsInitialized();
  static NewTabFeatureEngagementTracker* Get();

  NewTabFeatureEngagementTracker();
  ~NewTabFeatureEngagementTracker() override {
    duration_tracker_->RemoveObserver(observer_.release());
  }

  // Notifies the feature engagement tracker that a new tab was opened.
  void NotifyNewTabOpened();
  // Notifies the feature engagement tracker that the omnibox has been used.
  void NotifyOmniboxNavigation();
  // Notifies the feature engagement tracker that the session time is up.
  void NotifySessionTime();

 private:
  bool CheckSessionTime();
  void OnSessionEnded(base::TimeDelta delta) override;
  void UpdateSessionTime(base::TimeDelta elapsed);

  int elapsed_session_time_;
  //  duration_tracker_ is owned by NewTabFeatureEngagementTracker, and thus
  //  lives as long as the class.
  DesktopSessionDurationTracker* duration_tracker_;
  //  profile_manager_ is owned by NewTabFeatureEngagementTracker, and thus
  //  lives as long as the class.
  ProfileManager* profile_manager_;
  //  observer_ is released to duration_tracker_, and lives until it is
  //  removed by duration_tracker_.
  std::unique_ptr<NewTabFeatureEngagementTracker> observer_;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTracker);
};

}  // namespace feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
