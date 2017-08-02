// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace feature_engagement {

FeatureTracker::FeatureTracker(Profile* profile)
    : profile_(profile), duration_tracker_observer_(this) {
  AddDurationTrackerObserver();
}

void FeatureTracker::OnSessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(events::kSessionTime);
}

FeatureTracker::FeatureTracker() : FeatureTracker(nullptr) {}

FeatureTracker::~FeatureTracker() = default;

Tracker* FeatureTracker::GetFeatureTracker() {
  return TrackerFactory::GetForBrowserContext(profile_);
}

PrefService* FeatureTracker::GetPrefs() {
  return profile_->GetPrefs();
}

void FeatureTracker::AddDurationTrackerObserver() {
  duration_tracker_observer_.Add(metrics::DesktopSessionDurationTracker::Get());
}

void FeatureTracker::RemoveDurationTrackerObserver() {
  duration_tracker_observer_.Remove(
      metrics::DesktopSessionDurationTracker::Get());
}

void FeatureTracker::OnSessionEnded(base::TimeDelta delta) {
  if (HasEnoughSessionTimeElapsed()) {
    OnSessionTimeMet();
    RemoveDurationTrackerObserver();
  }
}

}  // namespace feature_engagement
