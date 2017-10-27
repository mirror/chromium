// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

#include "base/time/time.h"
#include "chrome/browser/feature_engagement/feature_promo_bubble.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace {

const int kDefaultPromoShowTimeInHours = 2;

}  // namespace

namespace feature_engagement {

NewTabTracker::NewTabTracker(Profile* profile,
                             SessionDurationUpdater* session_duration_updater)
    : FeatureTracker(profile,
                     session_duration_updater,
                     &kIPHNewTabFeature,
                     base::TimeDelta::FromHours(kDefaultPromoShowTimeInHours)) {
}

NewTabTracker::NewTabTracker(SessionDurationUpdater* session_duration_updater)
    : NewTabTracker(nullptr, session_duration_updater) {}

NewTabTracker::~NewTabTracker() = default;

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

void NewTabTracker::OnPromoClosed() {
  GetTracker()->Dismissed(kIPHNewTabFeature);
}

void NewTabTracker::OnSessionTimeMet() {
  GetTracker()->NotifyEvent(events::kNewTabSessionTimeMet);
}

void NewTabTracker::CloseBubble() {
  if (feature_promo_bubble()) {
    feature_promo_bubble()->ClosePromoBubble();
    set_feature_promo_bubble(nullptr);
  }
}

void NewTabTracker::ShowPromo() {
  DCHECK(!feature_promo_bubble());
  set_feature_promo_bubble(ShowNewTabFeaturePromoBubble(GetBrowser()));
}

}  // namespace feature_engagement
