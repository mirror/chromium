// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/incognito_window/incognito_window_tracker.h"

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

namespace {

const int kTwoHoursInMinutes = 120;

}  // namespace

namespace feature_engagement_tracker {

IncognitoWindowTracker::IncognitoWindowTracker(Profile* profile)
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

IncognitoWindowTracker::IncognitoWindowTracker()
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(nullptr) {}

IncognitoWindowTracker::~IncognitoWindowTracker() = default;

// static
void IncognitoWindowTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kIncognitoWindowInProductHelp, false);
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

void IncognitoWindowTracker::DismissIncognitoWindowTracker() {
  GetFeatureTracker()->Dismissed(kIPHIncognitoWindowFeature);
}

void IncognitoWindowTracker::OnIncognitoWindowOpened() {
  GetFeatureTracker()->NotifyEvent(events::kIncognitoWindowOpened);
}

void IncognitoWindowTracker::OnHistoryDeleted() {
  GetFeatureTracker()->NotifyEvent(events::kHistoryDeleted);
  if (ShouldShowPromo())
    ShowPromo();
}

void IncognitoWindowTracker::OnSessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(events::kSessionTime);
}

void IncognitoWindowTracker::ShowPromo() {
  GetPrefs()->SetBoolean(prefs::kIncognitoWindowInProductHelp, true);
  // TODO(crbug.com/748231): Call the promo.

  // Clears the flag for whether there is any in-product help being displayed.
  GetFeatureTracker()->Dismissed(kIPHIncognitoWindowFeature);
}

bool IncognitoWindowTracker::ShouldShowPromo() {
  return GetFeatureTracker()->ShouldTriggerHelpUI(kIPHIncognitoWindowFeature);
}

FeatureEngagementTracker* IncognitoWindowTracker::GetFeatureTracker() {
  return FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

PrefService* IncognitoWindowTracker::GetPrefs() {
  return profile_->GetPrefs();
}

bool IncognitoWindowTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

void IncognitoWindowTracker::UpdateSessionTime(base::TimeDelta elapsed) {
  // Session time does not need to be tracked anymore if the
  // in-product help has been shown already.
  // This prevents unnecessary interaction with prefs.
  if (GetPrefs()->GetBoolean(prefs::kIncognitoWindowInProductHelp))
    return;

  base::TimeDelta elapsed_session_time;
  elapsed_session_time += base::TimeDelta::FromMinutes(GetPrefs()->GetInteger(
                              prefs::kSessionTimeTotal)) +
                          elapsed;
  GetPrefs()->SetInteger(prefs::kSessionTimeTotal,
                         elapsed_session_time.InMinutes());
}

void IncognitoWindowTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (HasEnoughSessionTimeElapsed()) {
    OnSessionTimeMet();
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace feature_engagement_tracker
