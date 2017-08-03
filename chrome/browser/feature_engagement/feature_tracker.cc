// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"

namespace feature_engagement {

FeatureTracker::FeatureTracker(Tracker* tracker)
    : tracker_(tracker), duration_tracker_observer_(this) {
  AddDurationTrackerObserver();
}

FeatureTracker::FeatureTracker() : FeatureTracker(nullptr) {}

FeatureTracker::~FeatureTracker() = default;

Tracker* FeatureTracker::GetFeatureTracker() {
  return tracker_;
}

void FeatureTracker::AddDurationTrackerObserver() {
  duration_tracker_observer_.Add(metrics::DesktopSessionDurationTracker::Get());
}

void FeatureTracker::RemoveDurationTrackerObserver() {
  duration_tracker_observer_.Remove(
      metrics::DesktopSessionDurationTracker::Get());
}

void FeatureTracker::OnSessionEnded(base::TimeDelta delta) {
  // Depending on the order that DesktopSessionDurationTracker notifies its
  // observors of the session ending, the FeatureTracker may be one session
  // behind when looking up kSessionTimeTotal if FeatureTracker::OnSessionEnded
  // is called before SessionDurationUpdater::OnSessionEnded.
  if (HasEnoughSessionTimeElapsed()) {
    OnSessionTimeMet();
    RemoveDurationTrackerObserver();
  }
}

}  // namespace feature_engagement
