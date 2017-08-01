// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/new_tab/new_tab_tracker.h"

#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"

namespace {

const int kTwoHoursInMinutes = 120;

}  // namespace

namespace feature_engagement_tracker {

NewTabTracker::NewTabTracker(Profile* profile)
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

NewTabTracker::NewTabTracker()
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(nullptr) {}

NewTabTracker::~NewTabTracker() = default;

// static
void NewTabTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

void NewTabTracker::OnPromoClosed() {
  GetFeatureTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnNewTabOpened() {
  GetFeatureTracker()->NotifyEvent(events::kNewTabOpened);
}

void NewTabTracker::OnOmniboxNavigation() {
  GetFeatureTracker()->NotifyEvent(events::kOmniboxInteraction);
}

void NewTabTracker::OnSessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(events::kSessionTime);
}

void NewTabTracker::OnOmniboxFocused() {
  // If the flag for demo mode is enabled, then bypass the session time
  // requirement to make testing easier.
  if (IsIPHNewTabEnabled()) {
    OnSessionTimeMet();
  }

  if (ShouldShowPromo())
    ShowPromo();
}

bool NewTabTracker::ShouldShowPromo() {
  return GetFeatureTracker()->ShouldTriggerHelpUI(kIPHNewTabFeature);
}

void NewTabTracker::AddDurationTrackerObserver() {
  duration_tracker_observer_.Add(metrics::DesktopSessionDurationTracker::Get());
}

void NewTabTracker::RemoveDurationTrackerObserver() {
  duration_tracker_observer_.Remove(
      metrics::DesktopSessionDurationTracker::Get());
}

bool NewTabTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

bool NewTabTracker::IsIPHNewTabEnabled() {
  const std::string group_name =
      base::FieldTrialList::FindFullName("NewTabInProductHelp");
  return base::StartsWith(group_name, "Enabled",
                          base::CompareCase::INSENSITIVE_ASCII);
}

void NewTabTracker::ShowPromo() {
  BrowserView* browser = static_cast<BrowserView*>(
      BrowserList::GetInstance()->GetLastActive()->window());
  browser->tabstrip()->new_tab_button()->ShowPromo();
}

bool NewTabTracker::ShouldShowPromo() {
  return GetFeatureTracker()->ShouldTriggerHelpUI(kIPHNewTabFeature);
}

FeatureEngagementTracker* NewTabTracker::GetFeatureTracker() {
  return FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

PrefService* NewTabTracker::GetPrefs() {
  return profile_->GetPrefs();
}

bool NewTabTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

void NewTabTracker::UpdateSessionTime(base::TimeDelta elapsed) {
  // Session time does not need to be tracked anymore if the
  // in-product help has been shown already.
  // This prevents unnecessary interaction with prefs.
  if (GetFeatureTracker()->GetTriggerState(kIPHNewTabFeature) ==
      Tracker::TriggerState::HAS_BEEN_DISPLAYED)
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
    OnSessionTimeMet();
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace feature_engagement_tracker
