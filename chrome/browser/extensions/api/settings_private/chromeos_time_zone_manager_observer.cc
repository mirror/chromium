// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/chromeos_time_zone_manager_observer.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/system/timezone_resolver_manager.h"

namespace extensions {

GeneratedPrefTimeZoneManagerObserver::GeneratedPrefTimeZoneManagerObserver(
    const std::string& pref_name)
    : pref_name_(pref_name) {
  g_browser_process->platform_part()->GetTimezoneResolverManager()->AddObserver(
      this);
}

GeneratedPrefTimeZoneManagerObserver::~GeneratedPrefTimeZoneManagerObserver() {
  g_browser_process->platform_part()
      ->GetTimezoneResolverManager()
      ->RemoveObserver(this);
}

void GeneratedPrefTimeZoneManagerObserver::OnTimeZoneResolverUpdated() {
  NotifyObservers(pref_name_);
}

}  // namespace extensions
