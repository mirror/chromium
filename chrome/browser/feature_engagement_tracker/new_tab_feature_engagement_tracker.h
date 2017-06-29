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
  ~NewTabFeatureEngagementTracker() override;

  // Notifies the feature engagement tracker that a new tab was opened.
  virtual void NotifyNewTabOpened();
  // Notifies the feature engagement tracker that the omnibox has been used.
  virtual void NotifyOmniboxNavigation();
  // Notifies the feature engagement tracker that the session time is up.
  virtual void NotifySessionTime();

 protected:
  virtual Profile* GetProfile();
  virtual void ShowPromo();

 private:
  bool CheckSessionTime();
  void OnSessionEnded(base::TimeDelta delta) override;
  void UpdateSessionTime(base::TimeDelta elapsed);

  int elapsed_session_time_;
  // The pointer is owned by NewTabFeatureEngagementTracker.
  DesktopSessionDurationTracker* duration_tracker_;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTracker);
};

}  // namespace feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
