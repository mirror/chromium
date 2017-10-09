// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_in_product_help_session_duration_updater.h"

#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace feature_engagement {

NewTabInProductHelpSessionDurationUpdater::
    NewTabInProductHelpSessionDurationUpdater(PrefService* pref_service)
    : SessionDurationUpdater(pref_service) {}

NewTabInProductHelpSessionDurationUpdater::
    ~NewTabInProductHelpSessionDurationUpdater() = default;

// static
void NewTabInProductHelpSessionDurationUpdater::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  SessionDurationUpdater::RegisterProfilePrefs(
      registry, prefs::kNewTabInProductHelpObservedSessionTime);
}

void NewTabInProductHelpSessionDurationUpdater::OnSessionEnded(
    base::TimeDelta elapsed) {
  SessionDurationUpdater::OnSessionEndedForPerf(
      elapsed, prefs::kNewTabInProductHelpObservedSessionTime);
}

}  // namespace feature_engagement
