// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/incognito_window/incognito_window_in_product_help_session_duration_updater.h"

#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace feature_engagement {

IncognitoWindowInProductHelpSessionDurationUpdater::
    IncognitoWindowInProductHelpSessionDurationUpdater(
        PrefService* pref_service)
    : SessionDurationUpdater(pref_service) {}

IncognitoWindowInProductHelpSessionDurationUpdater::
    ~IncognitoWindowInProductHelpSessionDurationUpdater() = default;

// static
void IncognitoWindowInProductHelpSessionDurationUpdater::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  SessionDurationUpdater::RegisterProfilePrefs(
      registry, prefs::kIncognitoWindowInProductHelpObservedSessionTime);
}

void IncognitoWindowInProductHelpSessionDurationUpdater::OnSessionEnded(
    base::TimeDelta elapsed) {
  SessionDurationUpdater::OnSessionEndedForPerf(
      elapsed, prefs::kIncognitoWindowInProductHelpObservedSessionTime);
}

}  // namespace feature_engagement
