// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/pref_service_syncable_util.h"

#include <vector>

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/sync_preferences/pref_service_syncable.h"

#if defined(OS_ANDROID)
#include "components/proxy_config/proxy_config_pref_names.h"
#endif

sync_preferences::PrefServiceSyncable* PrefServiceSyncableFromProfile(
    Profile* profile) {
  return static_cast<sync_preferences::PrefServiceSyncable*>(
      profile->GetPrefs());
}

sync_preferences::PrefServiceSyncable* PrefServiceSyncableIncognitoFromProfile(
    Profile* profile) {
  return static_cast<sync_preferences::PrefServiceSyncable*>(
      profile->GetOffTheRecordPrefs());
}

sync_preferences::PrefServiceSyncable* CreateIncognitoPrefServiceSyncable(
    sync_preferences::PrefServiceSyncable* pref_service,
    PrefStore* incognito_extension_pref_store) {
  // List of keys that can be changed in the user prefs file by the incognito
  // profile.  Ideally there should be none of these so please keep to a minimum
  // neccesary.
  //
  // TODO(tibell): Many of these refer to writes that only happen in tests that
  // could be changed to not rely on such writes.
  std::vector<const char*> underlay_pref_names;
  underlay_pref_names.push_back("profile.content_settings.exceptions.cookies");
  underlay_pref_names.push_back(bookmarks::prefs::kShowBookmarkBar);
  underlay_pref_names.push_back(prefs::kAlternateErrorPagesEnabled);
  underlay_pref_names.push_back(prefs::kDevToolsPreferences);
  underlay_pref_names.push_back(prefs::kDownloadDefaultDirectory);
  underlay_pref_names.push_back(prefs::kDownloadDirUpgraded);
  underlay_pref_names.push_back(prefs::kIncognitoModeAvailability);
  underlay_pref_names.push_back(prefs::kPromptForDownload);
  underlay_pref_names.push_back(prefs::kSafeBrowsingScoutReportingEnabled);
  underlay_pref_names.push_back(prefs::kSupervisedUserId);
  return pref_service->CreateIncognitoPrefService(
      incognito_extension_pref_store, underlay_pref_names);
}
