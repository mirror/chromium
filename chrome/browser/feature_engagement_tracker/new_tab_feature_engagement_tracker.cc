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

namespace feature_engagement_tracker {

namespace {

const int kTwoHoursInSeconds = 7200;
NewTabFeatureEngagementTracker* n_instance = nullptr;

}  // namespace

void NewTabFeatureEngagementTracker::Initialize() {
  n_instance = new NewTabFeatureEngagementTracker();
}

bool NewTabFeatureEngagementTracker::IsInitialized() {
  return n_instance != nullptr;
}

NewTabFeatureEngagementTracker* NewTabFeatureEngagementTracker::Get() {
  DCHECK(n_instance);
  return n_instance;
}

NewTabFeatureEngagementTracker::NewTabFeatureEngagementTracker()
    : duration_tracker_(DesktopSessionDurationTracker::Get()),
      elapsed_session_time_(0),
      profile_manager_(g_browser_process->profile_manager()),
      observer_(std::unique_ptr<NewTabFeatureEngagementTracker>()) {
  // duration_tracker_ now assumes ownership of observer_
  duration_tracker_->AddObserver(observer_.release());
}

void NewTabFeatureEngagementTracker::NotifyNewTabOpened() {
  Profile* profile_ = profile_manager_->GetProfileByPath(
      profile_manager_->GetLastUsedProfileDir(
          profile_manager_->user_data_dir()));
  FeatureEngagementTracker* feature_tracker_ =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
  feature_tracker_->NotifyEvent(events::kNewTabOpened);
  if (feature_tracker_->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
    feature_tracker_->Dismissed(kIPHNewTabFeature);
  }
}

void NewTabFeatureEngagementTracker::NotifyOmniboxNavigation() {
  Profile* profile_ = profile_manager_->GetProfileByPath(
      profile_manager_->GetLastUsedProfileDir(
          profile_manager_->user_data_dir()));
  FeatureEngagementTracker* feature_tracker_ =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
  feature_tracker_->NotifyEvent(events::kOmniboxInteraction);
  if (feature_tracker_->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
    feature_tracker_->Dismissed(kIPHNewTabFeature);
  }
}

void NewTabFeatureEngagementTracker::NotifySessionTime() {
  Profile* profile_ = profile_manager_->GetProfileByPath(
      profile_manager_->GetLastUsedProfileDir(
          profile_manager_->user_data_dir()));
  FeatureEngagementTracker* feature_tracker_ =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
  feature_tracker_->NotifyEvent(events::kSessionTime);
  if (!ui::IsFullScreenMode()) {
    if (feature_tracker_->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
      feature_tracker_->Dismissed(kIPHNewTabFeature);
    }
  }
}

bool NewTabFeatureEngagementTracker::CheckSessionTime() {
  if (elapsed_session_time_ >= kTwoHoursInSeconds) {
    return true;
  }
  return false;
}

void NewTabFeatureEngagementTracker::UpdateSessionTime(
    base::TimeDelta elapsed) {
  Profile* profile_ = profile_manager_->GetProfileByPath(
      profile_manager_->GetLastUsedProfileDir(
          profile_manager_->user_data_dir()));
  PrefService* prefs = profile_->GetPrefs();
  elapsed_session_time_ +=
      prefs->GetInteger(prefs::kSessionTimeTotal) + elapsed.InSeconds();
  prefs->SetInteger(prefs::kSessionTimeTotal, elapsed_session_time_);
}

void NewTabFeatureEngagementTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (CheckSessionTime()) {
    // duration_tracker_ now assumes ownership of observer_
    duration_tracker_->RemoveObserver(observer_.release());
    NotifySessionTime();
  }
}

}  // namespace feature_engagement_tracker
