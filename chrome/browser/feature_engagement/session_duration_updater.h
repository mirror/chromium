// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_

#include "base/scoped_observer.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace feature_engagement {

// Updates the kSessionTimeTotal pref to reflect the total amount of active
// time. This allows subclasses of FeatureTracker to check how much active time
// has passed, and then show their respective promos accordingly. Double
// counting of time happened when the pref was udpated in FeatureTracker, as the
// multiple subclasses of FeatureTracker would all add the time from the session
// independently. When a session ends, DesktopSessionDurationTracker calls
// OnSessionEnding on all of its observers, which includes the
// SessionDurationUpdater, along with all of the subclasses of FeatureTracker
// SessionDurationUpdater::OnSessionEnded updates the kSessionTimeTotal pref,
// while the subclass's of FeatureTracker's ::OnSessionEnded look up this pref's
// interger value through GetSessionTimeTotal, in order to determine if their
// session time condition has been met.
//
// Note: Due to the unreliabilty of the order that DeskTopDurationTracker calls
// its observers during OnSessionEnded, the FeatureTracker subclasses may be one
// session's worth of time behind if they are called before
// SessionDurationUpdater::OnSessionEnded is called to update the pref value.
class SessionDurationUpdater
    : public metrics::DesktopSessionDurationTracker::Observer,
      public KeyedService {
 public:
  explicit SessionDurationUpdater(PrefService* pref_service);
  SessionDurationUpdater();
  ~SessionDurationUpdater() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns the total session time in minutes.
  int GetSessionTimeTotal();

 private:
  // metrics::DesktopSessionDurationtracker::Observer:
  void OnSessionEnded(base::TimeDelta delta) override;
  // Adds the DesktopSessionDurationTracker observer.
  void AddDurationTrackerObserver();
  // Removes the DesktopSessionDurationTracker observer.
  void RemoveDurationTrackerObserver();

  // Observes the DesktopSessionDurationTracker and notifies when a desktop
  // session starts and ends.
  ScopedObserver<metrics::DesktopSessionDurationTracker,
                 metrics::DesktopSessionDurationTracker::Observer>
      duration_tracker_observer_;

  // Owned by Profile manager.
  PrefService* const pref_service_;

  DISALLOW_COPY_AND_ASSIGN(SessionDurationUpdater);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_
