// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_

#include "base/time/time.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"

using metrics::DesktopSessionDurationTracker;

namespace feature_engagement_tracker {

class NewTabFeatureEngagementTracker {
 public:
  static void Initialize(Profile* profile_);
  static bool IsInitialized();
  static NewTabFeatureEngagementTracker* Get();

  explicit NewTabFeatureEngagementTracker(Profile* profile_);
  ~NewTabFeatureEngagementTracker();

  // Notifies the feature engagement tracker that a new tab was opened.
  void NotifyNewTab();
  // Notifies the feature engagement tracker that the omnibox has been used.
  void NotifyOmnibox();
  // Notifies the feature engagement tracker that the session time is up.
  void NotifySessionTime();

  class NewTabSessionObserver : public DesktopSessionDurationTracker::Observer {
   public:
    NewTabSessionObserver(DesktopSessionDurationTracker* duration_tracker,
                          NewTabFeatureEngagementTracker* new_tab_tracker,
                          Profile* profile);

    ~NewTabSessionObserver() override {
      duration_tracker_->RemoveObserver(this);
    }

    bool timer_fired() const { return timer_fired_; }

   private:
    void OnSessionEnded(base::TimeDelta delta) override;

    bool CheckSessionTime(base::TimeDelta elapsed);

    int elapsed_session_time_;
    bool timer_fired_;
    DesktopSessionDurationTracker* duration_tracker_;
    NewTabFeatureEngagementTracker* new_tab_tracker_;
    Profile* profile_;

    DISALLOW_COPY_AND_ASSIGN(NewTabSessionObserver);
  };

 private:
  DesktopSessionDurationTracker* duration_tracker_;
  FeatureEngagementTracker* feature_tracker_;
  NewTabSessionObserver* observer;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTracker);
};

}  // namespace feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_FEATURE_ENGAGEMENT_TRACKER_H_
