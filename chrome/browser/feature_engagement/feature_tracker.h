// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_TRACKER_H_

#include "base/scoped_observer.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/keyed_service/core/keyed_service.h"

namespace feature_engagement {

// The FeatureTracker provides a backend for displaying in-product help for the
// various features.
class FeatureTracker : public metrics::DesktopSessionDurationTracker::Observer,
                       public KeyedService {
 public:
  explicit FeatureTracker(Tracker* tracker);

 protected:
  FeatureTracker();
  ~FeatureTracker() override;

  // Returns whether the active session time of a user has elapsed more than the
  // required active sessiom time for the feature.
  virtual bool HasEnoughSessionTimeElapsed() = 0;
  // Alerts the feature tracker that the session time is up.
  virtual void OnSessionTimeMet() = 0;
  virtual Tracker* GetFeatureTracker();

 private:
  // Adds the DesktopSessionDurationTracker observer.
  void AddDurationTrackerObserver();
  // Removes the DesktopSessionDurationTracker observer.
  void RemoveDurationTrackerObserver();
  // metrics::DesktopSessionDurationtracker::Observer:
  void OnSessionEnded(base::TimeDelta delta) override;

  // Singleton instance.
  Tracker* const tracker_;

  // Observes the DesktopSessionDurationTracker and notifies when a desktop
  // session starts and ends.
  ScopedObserver<metrics::DesktopSessionDurationTracker,
                 metrics::DesktopSessionDurationTracker::Observer>
      duration_tracker_observer_;

  DISALLOW_COPY_AND_ASSIGN(FeatureTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_TRACKER_H_
