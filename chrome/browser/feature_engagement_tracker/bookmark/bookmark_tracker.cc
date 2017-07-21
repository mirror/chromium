// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/bookmark/bookmark_tracker.h"

#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"

namespace {

// TODO: Determine what time ellapsed we want for this feature. Default is two
// hours for now.
const int kTwoHoursInMinutes = 120;

}  // namespace

namespace feature_engagement_tracker {

BookmarkTracker::BookmarkTracker(Profile* profile)
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(profile) {
  duration_tracker_->AddObserver(this);
}

BookmarkTracker::BookmarkTracker()
    : duration_tracker_(metrics::DesktopSessionDurationTracker::Get()),
      profile_(nullptr) {}

BookmarkTracker::~BookmarkTracker() = default;

// static
void BookmarkTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kBookmarkInProductHelp, false);
  registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
}

void BookmarkTracker::DismissBookmarkTracker() {
  GetFeatureTracker()->Dismissed(kIPHBookmarkFeature);
}

void BookmarkTracker::OnBookmarkAdded() {
  GetFeatureTracker()->NotifyEvent(events::kBookmarkAdded);
}

void BookmarkTracker::OnSessionTimeMet() {
  GetFeatureTracker()->NotifyEvent(events::kSessionTime);
}

bool BookmarkTracker::ShouldShowPromo() {
  return GetFeatureTracker()->ShouldTriggerHelpUI(kIPHBookmarkFeature);
}

void BookmarkTracker::OnVisitedKnownURL() {
  if (ShouldShowPromo())
    ShowPromo();
}

bool BookmarkTracker::HasEnoughSessionTimeElapsed() {
  return GetPrefs()->GetInteger(prefs::kSessionTimeTotal) >= kTwoHoursInMinutes;
}

bool BookmarkTracker::IsIPHBookmarkEnabled() {
  const std::string group_name =
      base::FieldTrialList::FindFullName("BookmarkInProductHelp");
  return base::StartsWith(group_name, "Enabled",
                          base::CompareCase::INSENSITIVE_ASCII);
}

void BookmarkTracker::ShowPromo() {
  GetPrefs()->SetBoolean(prefs::kBookmarkInProductHelp, true);
  // TODO: Call the promo.

  // Clears the flag for whether there is any in-product help being displayed.
  GetFeatureTracker()->Dismissed(kIPHBookmarkFeature);
}

FeatureEngagementTracker* BookmarkTracker::GetFeatureTracker() {
  return FeatureEngagementTrackerFactory::GetForBrowserContext(profile_);
}

PrefService* BookmarkTracker::GetPrefs() {
  return profile_->GetPrefs();
}

void BookmarkTracker::UpdateSessionTime(base::TimeDelta elapsed) {
  // Session time does not need to be tracked anymore if the
  // in-product help has been shown already.
  // This prevents unnecessary interaction with prefs.
  if (GetPrefs()->GetBoolean(prefs::kBookmarkInProductHelp))
    return;

  base::TimeDelta elapsed_session_time;
  elapsed_session_time += base::TimeDelta::FromMinutes(GetPrefs()->GetInteger(
                              prefs::kSessionTimeTotal)) +
                          elapsed;
  GetPrefs()->SetInteger(prefs::kSessionTimeTotal,
                         elapsed_session_time.InMinutes());
}

void BookmarkTracker::OnSessionEnded(base::TimeDelta delta) {
  UpdateSessionTime(delta);
  if (HasEnoughSessionTimeElapsed()) {
    OnSessionTimeMet();
    duration_tracker_->RemoveObserver(this);
  }
}

}  // namespace feature_engagement_tracker
