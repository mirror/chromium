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
  GetTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnNewTabOpened() {
  GetTracker()->NotifyEvent(events::kNewTabOpened);
}

void NewTabTracker::OnOmniboxNavigation() {
  GetTracker()->NotifyEvent(events::kOmniboxInteraction);
}

void NewTabTracker::OnOmniboxFocused() {
  if (ShouldShowPromo())
    ShowPromo();
}

int NewTabTracker::GetSessionTimeRequiredToShowInMinutes() {
  return kTwoHoursInMinutes;
}

void NewTabTracker::OnSessionTimeMet() {
  GetTracker()->NotifyEvent(events::kNewTabSessionTimeMet);
}

bool NewTabTracker::ShouldShowPromo() {
  return GetTracker()->ShouldTriggerHelpUI(kIPHNewTabFeature);
}

void NewTabTracker::ShowPromo() {
  // TODO(crbug.com/737830): Call the promo.

  // Clears the flag for whether there is any in-product help being displayed.
  GetTracker()->Dismissed(kIPHNewTabFeature);
}

}  // namespace feature_engagement
