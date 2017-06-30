// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker.h"

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
#include "components/prefs/pref_service.h"

using metrics::DesktopSessionDurationTracker;

namespace {

const int kTwoHoursInMinutes = 120;
new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker* n_instance =
    nullptr;

}  // namespace

namespace new_tab_feature_engagement_tracker {

NewTabFeatureEngagementTracker::NewTabFeatureEngagementTracker(Profile* profile)
    : duration_tracker_(DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

void NewTabFeatureEngagementTracker::NotifyNewTabOpened() {
  GetFeatureTracker()->NotifyEvent(
      feature_engagement_tracker::events::kNewTabOpened);
  if (ShouldShowPromo())
    ShowPromo();
}

void NewTabFeatureEngagementTracker::NotifyOmniboxNavigation() {
  GetFeatureTracker()->NotifyEvent(
      feature_engagement_tracker::events::kOmniboxInteraction);
  if (ShouldShowPromo())
    ShowPromo();
}

void NewTabFeatureEngagementTracker::NotifySessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(
      feature_engagement_tracker::events::kSessionTime);
}

void NewTabFeatureEngagementTracker::ShowPromo() {
  // TODO(crbug.com/737830): call the promo

  // This line must be called when the UI dismisses.
  GetFeatureTracker()->Dismissed(feature_engagement_tracker::kIPHNewTabFeature);
  GetPrefs()->SetBoolean(prefs::kNewTabInProductHelp, true);
}

bool NewTabFeatureEngagementTracker::ShouldShowPromo() {
  if (GetFeatureTracker()->ShouldTriggerHelpUI(
          feature_engagement_tracker::kIPHNewTabFeature)) {
    return true;
  }
  return false;
}

feature_engagement_tracker::FeatureEngagementTracker*
NewTabFeatureEngagementTracker::GetFeatureTracker() {
  return FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

PrefService* NewTabFeatureEngagementTracker::GetPrefs() {
  return profile_->GetPrefs();
}

bool NewTabFeatureEngagementTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

void NewTabFeatureEngagementTracker::UpdateSessionTime(
    base::TimeDelta elapsed) {
  if (!(GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp))) {
    base::TimeDelta elapsed_session_time = base::TimeDelta::FromMinutes(0);
    elapsed_session_time += base::TimeDelta::FromMinutes(GetPrefs()->GetInteger(
                                prefs::kSessionTimeTotal)) +
                            elapsed;
    GetPrefs()->SetInteger(prefs::kSessionTimeTotal,
                           elapsed_session_time.InMinutes());
  }
}

void NewTabFeatureEngagementTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (HasEnoughSessionTimeElapsed()) {
    NotifySessionTimeMet();
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace new_tab_feature_engagement_tracker
