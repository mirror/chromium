// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_H_

#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace feature_engagement_tracker {

// The IncognitoWindowTracker provides a backend for displaying in-product help
// for incognito window feature.
class IncognitoWindowTracker
    : public metrics::DesktopSessionDurationTracker::Observer,
      public KeyedService {
 public:
  explicit IncognitoWindowTracker(Profile* profile);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Clears the flag for whether there is any in-product help being displayed.
  void DismissIncognitoWindowTracker();
  // Alerts the incognito window tracker that an incognito window was opened.
  void OnIncognitoWindowOpened();
  // Alerts the incognito window tracker that browsing history was deleted.
  void OnHistoryDeleted();
  // Alerts the incognito window tracker that the session time is up.
  void OnSessionTimeMet();
  // Returns whether or not the promo should be displayed.
  bool ShouldShowPromo();

 protected:
  IncognitoWindowTracker();
  ~IncognitoWindowTracker() override;

 private:
  // Returns whether the active session time of a user has elapsed
  // more than two hours.
  bool HasEnoughSessionTimeElapsed();

  // Returns whether the IncognitoWindowInProductHelp field trial is enabled.
  bool IsIPHIncognitoWindowEnabled();

  // Sets the IncognitoWindowInProductHelp pref to true and calls the Incognito
  // Window Promo.
  void ShowPromo();

  // Virtual to support mocking by unit tests.
  virtual FeatureEngagementTracker* GetFeatureTracker();

  virtual PrefService* GetPrefs();

  // Updates the pref that stores active session time per user unless the
  // incognito window in-product help has been displayed already.
  virtual void UpdateSessionTime(base::TimeDelta elapsed);

  // metrics::DesktopSessionDurationtracker::Observer::
  void OnSessionEnded(base::TimeDelta delta) override;

  // These pointers are owned by IncognitoWindowTracker.
  metrics::DesktopSessionDurationTracker* const duration_tracker_;

  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowTracker);
};

}  // namespace feature_engagement_tracker

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_TRACKER_INCOGNITO_WINDOW_INCOGNITO_WINDOW_TRACKER_H_
