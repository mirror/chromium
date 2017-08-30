// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/feature_tracker.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace feature_engagement {

FeatureTracker::FeatureTracker(Profile* profile,
                               SessionDurationUpdater* session_duration_updater,
                               const base::Feature* feature,
                               base::TimeDelta defaultTimeRequiredToShowPromo)
    : profile_(profile),
      session_duration_updater_(session_duration_updater),
      session_duration_observer_(this),
      feature_(feature),
      field_trial_time_delta_(defaultTimeRequiredToShowPromo) {
  AddSessionDurationObserver();
}

FeatureTracker::~FeatureTracker() = default;

void FeatureTracker::AddSessionDurationObserver() {
  session_duration_observer_.Add(session_duration_updater_);
}

void FeatureTracker::RemoveSessionDurationObserver() {
  session_duration_observer_.Remove(session_duration_updater_);
}

bool FeatureTracker::IsObserving() {
  return session_duration_observer_.IsObserving(session_duration_updater_);
}

bool FeatureTracker::ShouldShowPromo() {
  return GetTracker()->ShouldTriggerHelpUI(*feature_);
}

Tracker* FeatureTracker::GetTracker() const {
  return TrackerFactory::GetForBrowserContext(profile_);
}

void FeatureTracker::OnSessionEnded(base::TimeDelta total_session_time) {
  if (HasEnoughSessionTimeElapsed(total_session_time)) {
    OnSessionTimeMet();
    RemoveSessionDurationObserver();
  }
}

base::TimeDelta FeatureTracker::GetSessionTimeRequiredToShow() {
  if (!has_retrieved_field_trial_minutes_) {
    has_retrieved_field_trial_minutes_ = true;
    std::string field_trial_value =
        base::GetFieldTrialParamValueByFeature(*feature_, "x_minutes");
    if (!field_trial_value.empty()) {
      field_trial_time_delta_ =
          base::TimeDelta::FromMinutes(std::stoi(field_trial_value, nullptr));
    }
  }
  return field_trial_time_delta_;
}

bool FeatureTracker::HasEnoughSessionTimeElapsed(
    base::TimeDelta total_session_time) {
  return total_session_time.InMinutes() >=
         GetSessionTimeRequiredToShow().InMinutes();
}

}  // namespace feature_engagement
