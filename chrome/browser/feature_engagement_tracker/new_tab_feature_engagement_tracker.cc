// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/new_tab_feature_engagement_tracker.h"

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
feature_engagement_tracker::NewTabFeatureEngagementTracker* n_instance =
    nullptr;

}  // namespace

namespace feature_engagement_tracker {

// static
void NewTabFeatureEngagementTracker::Initialize() {
  n_instance = new NewTabFeatureEngagementTracker();
}

// static
bool NewTabFeatureEngagementTracker::IsInitialized() {
  return n_instance != nullptr;
}

// static
NewTabFeatureEngagementTracker* NewTabFeatureEngagementTracker::Get() {
  DCHECK(n_instance);
  return n_instance;
}

NewTabFeatureEngagementTracker::NewTabFeatureEngagementTracker()
    : duration_tracker_(DesktopSessionDurationTracker::Get()),
      elapsed_session_time_(0) {
  duration_tracker_->AddObserver(this);
}

void NewTabFeatureEngagementTracker::NotifyNewTabOpened() {
  FeatureEngagementTracker* feature_tracker =
      FeatureEngagementTrackerFactory::GetForBrowserContext(GetProfile());
  feature_tracker->NotifyEvent(events::kNewTabOpened);
  if (feature_tracker->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
    ShowPromo();
    feature_tracker->Dismissed(kIPHNewTabFeature);
  }
}

void NewTabFeatureEngagementTracker::NotifyOmniboxNavigation() {
  FeatureEngagementTracker* feature_tracker =
      FeatureEngagementTrackerFactory::GetForBrowserContext(GetProfile());
  feature_tracker->NotifyEvent(events::kOmniboxInteraction);
  if (feature_tracker->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
    ShowPromo();
    feature_tracker->Dismissed(kIPHNewTabFeature);
  }
}

void NewTabFeatureEngagementTracker::NotifySessionTime() {
  FeatureEngagementTracker* feature_tracker =
      FeatureEngagementTrackerFactory::GetForBrowserContext(GetProfile());
  feature_tracker->NotifyEvent(events::kSessionTime);
  if (!ui::IsFullScreenMode()) {
    if (feature_tracker->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
      ShowPromo();
      feature_tracker->Dismissed(kIPHNewTabFeature);
    }
  }
}

Profile* NewTabFeatureEngagementTracker::GetProfile() {
  return ProfileManager::GetLastUsedProfile();
}

void NewTabFeatureEngagementTracker::ShowPromo() {
  // TODO: chrisbenson
  // This is where the promo will be called.
}

bool NewTabFeatureEngagementTracker::CheckSessionTime() {
  return elapsed_session_time_ >= kTwoHoursInSeconds;
}

void NewTabFeatureEngagementTracker::UpdateSessionTime(
    base::TimeDelta elapsed) {
  PrefService* prefs = GetProfile()->GetPrefs();
  if (!prefs::kNewTabInProductHelp) {
    prefs->SetBoolean(prefs::kNewTabInProductHelp, true);
    elapsed_session_time_ +=
        prefs->GetInteger(prefs::kSessionTimeTotal) + elapsed.InSeconds();
    prefs->SetInteger(prefs::kSessionTimeTotal, elapsed_session_time_);
  }
}

void NewTabFeatureEngagementTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (CheckSessionTime()) {
    duration_tracker_->RemoveObserver(this);
    NotifySessionTime();
  }
}

}  // namespace feature_engagement_tracker
