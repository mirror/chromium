// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab/new_tab_tracker.h"

#include "base/metrics/field_trial.h"
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

NewTabTracker::NewTabTracker(Profile* profile) : FeatureTracker(profile) {}

void NewTabTracker::DismissNewTabTracker() {
  GetFeatureEngagementTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnNewTabOpened() {
  GetFeatureEngagementTracker()->NotifyEvent(events::kNewTabOpened);
}

void NewTabTracker::OnOmniboxNavigation() {
  GetFeatureEngagementTracker()->NotifyEvent(events::kOmniboxInteraction);
}

void NewTabTracker::OnOmniboxFocused() {
  if (ShouldShowPromo())
    ShowPromo();
}

// static
void NewTabTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kNewTabInProductHelp, false);
}

bool NewTabTracker::ShouldShowPromo() {
  return GetFeatureEngagementTracker()->ShouldTriggerHelpUI(kIPHNewTabFeature);
}

bool NewTabTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

void NewTabTracker::ShowPromo() {
  GetPrefs()->SetBoolean(prefs::kNewTabInProductHelp, true);
  // TODO(crbug.com/737830): Call the promo.

  // Clears the flag for whether there is any in-product help being displayed.
  GetFeatureEngagementTracker()->Dismissed(kIPHNewTabFeature);
}

bool NewTabTracker::IsIPHNewTabEnabled() {
  const std::string group_name =
      base::FieldTrialList::FindFullName("NewTabInProductHelp");
  return base::StartsWith(group_name, "Enabled",
                          base::CompareCase::INSENSITIVE_ASCII);
}

}  // namespace feature_engagement_tracker
