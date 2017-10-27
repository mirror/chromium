// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/incognito_window/incognito_window_tracker.h"

#include "chrome/browser/feature_engagement/feature_promo_bubble.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/ui/toolbar/app_menu_icon_controller.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"

namespace {

constexpr int kDefaultPromoShowTimeInHours = 2;

}  // namespace

namespace feature_engagement {

IncognitoWindowTracker::IncognitoWindowTracker(
    Profile* profile,
    SessionDurationUpdater* session_duration_updater)
    : FeatureTracker(profile,
                     session_duration_updater,
                     &kIPHIncognitoWindowFeature,
                     base::TimeDelta::FromHours(kDefaultPromoShowTimeInHours)) {
}

IncognitoWindowTracker::IncognitoWindowTracker(
    SessionDurationUpdater* session_duration_updater)
    : IncognitoWindowTracker(nullptr, session_duration_updater) {}
IncognitoWindowTracker::~IncognitoWindowTracker() = default;

void IncognitoWindowTracker::OnIncognitoWindowOpened() {
  GetTracker()->NotifyEvent(events::kIncognitoWindowOpened);
}

void IncognitoWindowTracker::OnBrowsingDataCleared() {
  const auto severity = GetAppMenuButtonSeverity(GetBrowser());
  if (severity == AppMenuIconController::Severity::NONE && ShouldShowPromo())
    ShowPromo();
}

void IncognitoWindowTracker::OnPromoClosed() {
  GetTracker()->Dismissed(kIPHIncognitoWindowFeature);
}

void IncognitoWindowTracker::OnSessionTimeMet() {
  GetTracker()->NotifyEvent(events::kIncognitoWindowSessionTimeMet);
}

void IncognitoWindowTracker::ShowPromo() {
  DCHECK(!feature_promo_bubble());
  set_feature_promo_bubble(ShowIncognitoFeaturePromoBubble(GetBrowser()));
}

}  // namespace feature_engagement
