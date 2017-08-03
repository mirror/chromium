// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/session_duration_updater.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace feature_engagement {

SessionDurationUpdater::SessionDurationUpdater(PrefService* pref_service)
    : duration_tracker_observer_(this), pref_service_(pref_service) {
  AddDurationTrackerObserver();
}

SessionDurationUpdater::SessionDurationUpdater()
    : SessionDurationUpdater(nullptr) {}

SessionDurationUpdater::~SessionDurationUpdater() = default;

// static
void SessionDurationUpdater::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

int SessionDurationUpdater::GetSessionTimeTotal() {
  return pref_service_->GetInteger(prefs::kSessionTimeTotal);
}

void SessionDurationUpdater::OnSessionEnded(base::TimeDelta elapsed) {
  base::TimeDelta elapsed_session_time;
  elapsed_session_time +=
      base::TimeDelta::FromMinutes(
          pref_service_->GetInteger(prefs::kSessionTimeTotal)) +
      elapsed;
  pref_service_->SetInteger(prefs::kSessionTimeTotal,
                            elapsed_session_time.InMinutes());
  RemoveDurationTrackerObserver();
}

void SessionDurationUpdater::AddDurationTrackerObserver() {
  duration_tracker_observer_.Add(metrics::DesktopSessionDurationTracker::Get());
}

void SessionDurationUpdater::RemoveDurationTrackerObserver() {
  duration_tracker_observer_.Remove(
      metrics::DesktopSessionDurationTracker::Get());
}

}  // namespace feature_engagement
