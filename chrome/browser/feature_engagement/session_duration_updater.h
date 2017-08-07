// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_

#include "base/scoped_observer.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"

class PrefRegistrySimple;

namespace feature_engagement {

// Updates the kObservedSessionTime pref to reflect the total amount of active
// time across Chrome restarts. This allows subclasses of FeatureTracker to
// check how much active time has passed, and then decide whether to show their
// respective promos accordingly.
//
// When an active session is closed, DesktopSessionDurationTracker calls
// OnSessionEnded and the kObservedSessionTime is incremented. Then,
// OnSessionEnded is called on all of Features observing SessionDurationUpdater.
// If the feature has its time limit exceeded, it should remove itself as an
// observer of SessionDurationUpdater. If SessionDurationUpdater has no
// observers, it means that the time limits of all the observing features has
// been exceeded, so the SessionDurationUpdater removes itself as an observer of
// DesktopSessionDurationTracker and stops updating kObservedSessionTime.
//
// If all observers are removed, SessionDurationUpdater doesn't continue
// updating kObservedSessionTime. However, if another feature is added as an
// observer later, the kObservedSessionTime pref is set back to 0, and
// SessionDurationUpdater begins updating kObservedSessionTime again.

class SessionDurationUpdater
    : public metrics::DesktopSessionDurationTracker::Observer,
      public KeyedService {
 public:
  // The methods for the observer will be called on the UI thread.
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void OnSessionEnded(base::TimeDelta total_session_time) = 0;
  };

  explicit SessionDurationUpdater(PrefService* pref_service);
  ~SessionDurationUpdater() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Returns the total session time in minutes.
  int GetSessionTimeTotal();

  // For observing the status of the session tracker.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

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

  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(SessionDurationUpdater);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_SESSION_DURATION_UPDATER_H_
