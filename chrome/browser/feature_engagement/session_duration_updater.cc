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
#include "components/prefs/pref_service.h"

namespace feature_engagement {

SessionDurationUpdater::SessionDurationUpdater(Profile* profile)
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

// static
void SessionDurationUpdater::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

SessionDurationUpdater::SessionDurationUpdater()
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(nullptr) {}

SessionDurationUpdater::~SessionDurationUpdater() = default;

PrefService* SessionDurationUpdater::GetPrefs() {
  return profile_->GetPrefs();
}

void SessionDurationUpdater::OnSessionEnded(base::TimeDelta elapsed) {
  base::TimeDelta elapsed_session_time;
  elapsed_session_time += base::TimeDelta::FromMinutes(GetPrefs()->GetInteger(
                              prefs::kSessionTimeTotal)) +
                          elapsed;
  GetPrefs()->SetInteger(prefs::kSessionTimeTotal,
                         elapsed_session_time.InMinutes());
}

}  // namespace feature_engagement
