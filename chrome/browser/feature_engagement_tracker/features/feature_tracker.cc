// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/feature_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace feature_engagement_tracker {

FeatureTracker::FeatureTracker(Profile* profile)
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

// static
void FeatureTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

void FeatureTracker::OnSessionTimeMet() {
  GetFeatureEngagementTracker()->NotifyEvent(events::kSessionTime);
}

FeatureTracker::FeatureTracker()
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(nullptr) {}

FeatureTracker::~FeatureTracker() = default;

FeatureEngagementTracker* FeatureTracker::GetFeatureEngagementTracker() {
  return FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

PrefService* FeatureTracker::GetPrefs() {
  return profile_->GetPrefs();
}

void FeatureTracker::UpdateSessionTime(base::TimeDelta elapsed) {
  base::TimeDelta elapsed_session_time;
  elapsed_session_time += base::TimeDelta::FromMinutes(GetPrefs()->GetInteger(
                              prefs::kSessionTimeTotal)) +
                          elapsed;
  GetPrefs()->SetInteger(prefs::kSessionTimeTotal,
                         elapsed_session_time.InMinutes());
}

void FeatureTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (HasEnoughSessionTimeElapsed()) {
    OnSessionTimeMet();
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace feature_engagement_tracker
