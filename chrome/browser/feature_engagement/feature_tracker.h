// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_TRACKER_H_

#include "base/scoped_observer.h"
#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "components/keyed_service/core/keyed_service.h"

namespace feature_engagement {

class Tracker;

// The FeatureTracker provides a backend for displaying in-product help for the
// various features. All subclasses of FeatureTracker's factories depend on
// SessionDurationUpdaterFactory::GetInstance() as SessionDurationUpdater is
// responsible for letting all FeatureTrackers know how much active session time
// has passed, which is a factor in whether or not the feature can be shown.
class FeatureTracker : public SessionDurationUpdater::Observer,
                       public KeyedService {
 public:
  FeatureTracker(Tracker* tracker,
                 SessionDurationUpdater* session_duration_updater);

  // Adds the SessionDurationUpdater observer.
  void AddSessionDurationObserver();
  // Removes the SessionDurationUpdater observer.
  void RemoveSessionDurationObserver();

 protected:
  ~FeatureTracker() override;

  virtual int GetSessionTimeRequiredToShowInMinutes() = 0;
  // Alerts the feature tracker that the session time is up.
  virtual void OnSessionTimeMet() = 0;
  virtual Tracker* GetFeatureTracker() const;

 private:
  // SessionDurationUpdater::Observer:
  void OnSessionEnded(base::TimeDelta total_session_time) override;

  // Returns whether the active session time of a user has elapsed more than the
  // required active session time for the feature.
  bool HasEnoughSessionTimeElapsed(base::TimeDelta total_session_time);

  // Singleton instance of a KeyedService.
  Tracker* const tracker_;

  // Singleton instance of KeyedService.
  SessionDurationUpdater* const session_duration_updater_;

  // Observes the SessionDurationUpdater and notifies when a desktop session
  // starts and ends.
  ScopedObserver<SessionDurationUpdater, SessionDurationUpdater::Observer>
      session_duration_observer_;

  DISALLOW_COPY_AND_ASSIGN(FeatureTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_TRACKER_H_
