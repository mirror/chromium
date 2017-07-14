// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_TRACKER_H_

#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace new_tab_help {

// The NewTabTracker provides a backend for displaying
// in-product help for the new tab button.
class NewTabTracker : public metrics::DesktopSessionDurationTracker::Observer,
                      public KeyedService {
 public:
  explicit NewTabTracker(Profile* profile);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Alerts the new tab tracker that a new tab was opened.
  void OnNewTabOpened();
  // Alerts the new tab tracker that the omnibox has been used.
  void OnOmniboxNavigation();
  // Alerts the new tab tracker that the session time is up.
  void OnSessionTimeMet();
  // Checks if the promo should be displayed since the omnibox is on focus.
  void OnOmniboxFocused();
  // Returns whether or not the promo should be displayed.
  bool ShouldShowPromo();

 protected:
  NewTabTracker();
  ~NewTabTracker() override;

 private:
  // Returns whether the active session time of a user has elapsed
  // more than two hours.
  bool HasEnoughSessionTimeElapsed();

  void ShowPromo();

  // Virtual to support mocking by unit tests.
  virtual feature_engagement_tracker::FeatureEngagementTracker*
  GetFeatureTracker();

  virtual PrefService* GetPrefs();

  virtual void UpdateSessionTime(base::TimeDelta elapsed);

  // metrics::DesktopSessionDurationtracker::Observer::
  void OnSessionEnded(base::TimeDelta delta) override;

  // These pointers are owned by NewTabTracker.
  metrics::DesktopSessionDurationTracker* const duration_tracker_;

  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(NewTabTracker);
};

}  // namespace new_tab_help

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_FEATURES_NEW_TAB_TRACKER_H_
