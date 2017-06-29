// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker.h"

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
#include "ui/base/fullscreen_win.h"

using metrics::DesktopSessionDurationTracker;

namespace {

const int kTwoHoursInSeconds = 7200;
new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker* n_instance =
    nullptr;

}  // namespace

namespace new_tab_feature_engagement_tracker {

NewTabFeatureEngagementTracker::NewTabFeatureEngagementTracker()
    : elapsed_session_time_(0),
      duration_tracker_(DesktopSessionDurationTracker::Get()) {
  duration_tracker_->AddObserver(this);
}

void NewTabFeatureEngagementTracker::NotifyNewTabOpened(Profile* profile) {
  feature_engagement_tracker::FeatureEngagementTracker* feature_tracker =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile);
  feature_tracker->NotifyEvent(
      feature_engagement_tracker::events::kNewTabOpened);
  TryShowPromo(feature_tracker);
}

void NewTabFeatureEngagementTracker::NotifyOmniboxNavigation(Profile* profile) {
  feature_engagement_tracker::FeatureEngagementTracker* feature_tracker =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile);
  feature_tracker->NotifyEvent(
      feature_engagement_tracker::events::kOmniboxInteraction);
  TryShowPromo(feature_tracker);
}

void NewTabFeatureEngagementTracker::NotifySessionTimeMet(Profile* profile) {
  feature_engagement_tracker::FeatureEngagementTracker* feature_tracker =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile);
  feature_tracker->NotifyEvent(
      feature_engagement_tracker::events::kSessionTime);
  if (!ui::IsFullScreenMode()) {
    TryShowPromo(feature_tracker);
  }
}

void NewTabFeatureEngagementTracker::TryShowPromo(
    feature_engagement_tracker::FeatureEngagementTracker* feature_tracker) {
  if (feature_tracker->ShouldTriggerHelpUI(
          feature_engagement_tracker::kIPHNewTabFeature)) {
    // TODO(crbug.com/737830): call the promo
    feature_tracker->Dismissed(feature_engagement_tracker::kIPHNewTabFeature);
  }
}

bool NewTabFeatureEngagementTracker::HasEnoughSessionTimeElapsed() {
  return elapsed_session_time_ >= kTwoHoursInSeconds;
}

void NewTabFeatureEngagementTracker::UpdateSessionTime(base::TimeDelta elapsed,
                                                       Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  if (!prefs->GetBoolean(prefs::kNewTabInProductHelp)) {
    prefs->SetBoolean(prefs::kNewTabInProductHelp, true);
    elapsed_session_time_ +=
        prefs->GetInteger(prefs::kSessionTimeTotal) + elapsed.InSeconds();
    prefs->SetInteger(prefs::kSessionTimeTotal, elapsed_session_time_);
  }
}

void NewTabFeatureEngagementTracker::OnSessionEnded(base::TimeDelta delta) {
  Profile* profile = ProfileManager::GetLastUsedProfile();
  UpdateSessionTime(delta, profile);
  if (HasEnoughSessionTimeElapsed()) {
    NotifySessionTimeMet(profile);
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace new_tab_feature_engagement_tracker
