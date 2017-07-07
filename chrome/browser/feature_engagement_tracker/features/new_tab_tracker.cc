// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_tracker.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

using metrics::DesktopSessionDurationTracker;

namespace {

const int kTwoHoursInMinutes = 120;

}  // namespace

namespace new_tab_iph {

NewTabTracker::NewTabTracker(Profile* profile)
    : duration_tracker_(DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

NewTabTracker::NewTabTracker() = default;

NewTabTracker::~NewTabTracker() = default;

void NewTabTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kNewTabInProductHelp, false);
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

void NewTabTracker::NotifyNewTabOpened() {
  GetFeatureTracker()->NotifyEvent(
      feature_engagement_tracker::events::kNewTabOpened);
  if (ShouldShowPromo())
    ShowPromo();
}

void NewTabTracker::NotifyOmniboxNavigation() {
  GetFeatureTracker()->NotifyEvent(
      feature_engagement_tracker::events::kOmniboxInteraction);
  if (ShouldShowPromo())
    ShowPromo();
}

void NewTabTracker::NotifySessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(
      feature_engagement_tracker::events::kSessionTime);
}

void NewTabTracker::ShowPromo() {
  // TODO(crbug.com/737830): call the promo

  // This line must be called when the UI dismisses.
  GetFeatureTracker()->Dismissed(feature_engagement_tracker::kIPHNewTabFeature);
  GetPrefs()->SetBoolean(prefs::kNewTabInProductHelp, true);
}

bool NewTabTracker::ShouldShowPromo() {
  return GetFeatureTracker()->ShouldTriggerHelpUI(
      feature_engagement_tracker::kIPHNewTabFeature);
}

feature_engagement_tracker::FeatureEngagementTracker*
NewTabTracker::GetFeatureTracker() {
  return FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

PrefService* NewTabTracker::GetPrefs() {
  return profile_->GetPrefs();
}

bool NewTabTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

void NewTabTracker::UpdateSessionTime(base::TimeDelta elapsed) {
  // If the in product help has been shown already, session time does not
  // need to be tracked anymore.
  if (GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp))
    return;

  base::TimeDelta elapsed_session_time;
  elapsed_session_time += base::TimeDelta::FromMinutes(GetPrefs()->GetInteger(
                              prefs::kSessionTimeTotal)) +
                          elapsed;
  GetPrefs()->SetInteger(prefs::kSessionTimeTotal,
                         elapsed_session_time.InMinutes());
}

void NewTabTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (HasEnoughSessionTimeElapsed()) {
    NotifySessionTimeMet();
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace new_tab_iph
