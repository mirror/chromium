// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/browser/feature_engagement/tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace {

const int kTwoHoursInMinutes = 120;

}  // namespace

namespace feature_engagement {

NewTabTracker::NewTabTracker(Tracker* tracker,
                             SessionDurationUpdater* session_duration_updater)
    : FeatureTracker(tracker, session_duration_updater) {}

NewTabTracker::NewTabTracker(SessionDurationUpdater* session_duration_updater)
    : NewTabTracker(nullptr, session_duration_updater) {}

NewTabTracker::~NewTabTracker() = default;

void NewTabTracker::DismissNewTabTracker() {
  GetFeatureTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnNewTabOpened() {
  GetFeatureTracker()->NotifyEvent(events::kNewTabOpened);
}

void NewTabTracker::OnOmniboxNavigation() {
  GetFeatureTracker()->NotifyEvent(events::kOmniboxInteraction);
}

void NewTabTracker::OnOmniboxFocused() {
  if (ShouldShowPromo())
    ShowPromo();
}

bool NewTabTracker::ShouldShowPromo() {
  return GetFeatureTracker()->ShouldTriggerHelpUI(kIPHNewTabFeature);
}

void NewTabTracker::OnSessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(events::kNewTabSessionTimeMet);
}

int NewTabTracker::GetSessionTimeRequiredToShowInMinutes() {
  return kTwoHoursInMinutes;
}

void NewTabTracker::ShowPromo() {
  // TODO(crbug.com/737830): Call the promo.

  // Clears the flag for whether there is any in-product help being displayed.
  GetFeatureTracker()->Dismissed(kIPHNewTabFeature);
}

bool NewTabTracker::IsIPHNewTabEnabled() {
  const std::string group_name =
      base::FieldTrialList::FindFullName("NewTabInProductHelp");
  return base::StartsWith(group_name, "Enabled",
                          base::CompareCase::INSENSITIVE_ASCII);
}

}  // namespace feature_engagement
