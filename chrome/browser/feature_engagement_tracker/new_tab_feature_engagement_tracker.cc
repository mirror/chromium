// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/new_tab_feature_engagement_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
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

void NewTabFeatureEngagementTracker::Initialize(Profile* profile_) {
  n_instance = new NewTabFeatureEngagementTracker(profile_);
}

bool NewTabFeatureEngagementTracker::IsInitialized() {
  return n_instance != nullptr;
}

NewTabFeatureEngagementTracker* NewTabFeatureEngagementTracker::Get() {
  DCHECK(n_instance);
  return n_instance;
}

NewTabFeatureEngagementTracker::NewTabFeatureEngagementTracker(Profile* profile)
    : profile_(profile) {
  duration_tracker_ = DesktopSessionDurationTracker::Get();
  observer = new NewTabSessionObserver(duration_tracker_, this, profile_);
  duration_tracker_->AddObserver(observer);
  feature_tracker_ =
      FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

NewTabFeatureEngagementTracker::NewTabSessionObserver::NewTabSessionObserver(
    DesktopSessionDurationTracker* duration_tracker,
    NewTabFeatureEngagementTracker* new_tab_tracker,
    Profile* profile)
    : elapsed_session_time_(0),
      timer_fired_(false),
      duration_tracker_(duration_tracker),
      new_tab_tracker_(new_tab_tracker),
      profile_(profile) {}

void NewTabFeatureEngagementTracker::NewTabSessionObserver::OnSessionEnded(
    base::TimeDelta delta) {
  if (CheckSessionTime(delta)) {
    timer_fired_ = true;
    duration_tracker_->RemoveObserver(this);
    new_tab_tracker_->NotifySessionTime();
  }
}

bool NewTabFeatureEngagementTracker::NewTabSessionObserver::CheckSessionTime(
    base::TimeDelta elapsed) {
  PrefService* prefs = profile_->GetPrefs();
  elapsed_session_time_ +=
      prefs->GetInteger(prefs::kSessionTimeTotal) + elapsed.InSeconds();
  prefs->SetInteger(prefs::kSessionTimeTotal, elapsed_session_time_);
  if (elapsed_session_time_ >= kTwoHoursInSeconds) {
    return true;
  }
  return false;
}

void NewTabFeatureEngagementTracker::NotifyNewTab() {
  feature_tracker_->NotifyEvent(events::kNewTabOpened);
  if (feature_tracker_->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
    feature_tracker_->Dismissed(kIPHNewTabFeature);
  }
}

void NewTabFeatureEngagementTracker::NotifyOmnibox() {
  feature_tracker_->NotifyEvent(events::kOmniboxInteraction);
  if (feature_tracker_->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
    feature_tracker_->Dismissed(kIPHNewTabFeature);
  }
}

void NewTabFeatureEngagementTracker::NotifySessionTime() {
  feature_tracker_->NotifyEvent(events::kSessionTime);
  if (!ui::IsFullScreenMode()) {
    if (feature_tracker_->ShouldTriggerHelpUI(kIPHNewTabFeature)) {
      feature_tracker_->Dismissed(kIPHNewTabFeature);
    }
  }
}

}  // namespace feature_engagement_tracker
