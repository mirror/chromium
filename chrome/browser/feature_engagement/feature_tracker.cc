// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace feature_engagement {

FeatureTracker::FeatureTracker(Tracker* tracker,
                               SessionDurationUpdater* session_duration_updater)
    : tracker_(tracker),
      session_duration_observer_(this),
      session_duration_updater_(session_duration_updater) {
  AddSessionDurationObserver();
}

FeatureTracker::~FeatureTracker() = default;

Tracker* FeatureTracker::GetFeatureTracker() {
  return tracker_;
}

void FeatureTracker::AddSessionDurationObserver() {
  session_duration_observer_.Add(session_duration_updater_);
}

void FeatureTracker::RemoveSessionDurationObserver() {
  session_duration_observer_.Remove(session_duration_updater_);
}

void FeatureTracker::OnSessionEnded(const int total_session_time) {
  if (HasEnoughSessionTimeElapsed(total_session_time)) {
    OnSessionTimeMet();
    RemoveSessionDurationObserver();
  }
}

}  // namespace feature_engagement
