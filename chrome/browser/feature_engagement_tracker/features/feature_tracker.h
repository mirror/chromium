// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_NEW_TAB_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_NEW_TAB_TRACKER_H_

#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace feature_engagement_tracker {

// The FeatureTracker provides a backend for displaying
// in-product help for the various features.
class FeatureTracker : public metrics::DesktopSessionDurationTracker::Observer,
                       public KeyedService {
 public:
  explicit FeatureTracker(Profile* profile);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Alerts the new tab tracker that the session time is up.
  void OnSessionTimeMet();
  // Returns whether or not the promo should be displayed.
  virtual bool ShouldShowPromo() = 0;

 protected:
  FeatureTracker();
  ~FeatureTracker() override;

  // Returns whether the active session time of a user has elapsed more than the
  // required active sessiom time for the feature.
  virtual bool HasEnoughSessionTimeElapsed() = 0;

  // Sets the feature's InProductHelp pref to true and calls the feature .
  virtual void ShowPromo() = 0;

  // Virtual to support mocking by unit tests.
  virtual FeatureEngagementTracker* GetFeatureEngagementTracker();
  virtual PrefService* GetPrefs();

 private:
  // Updates the pref that stores active session time per user unless the
  // new tab in-product help has been displayed already.
  void UpdateSessionTime(base::TimeDelta elapsed);

  // metrics::DesktopSessionDurationtracker::Observer::
  void OnSessionEnded(base::TimeDelta delta) override;

  // Assists in keeping a running total of active time for the Profile. Owned by
  // the browser.
  metrics::DesktopSessionDurationTracker* duration_tracker_;

  // Owned by Profile manager.
  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(FeatureTracker);
};

}  // namespace feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_NEW_TAB_NEW_TAB_TRACKER_H_
