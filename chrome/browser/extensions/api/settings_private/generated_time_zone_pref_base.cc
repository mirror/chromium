// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/generated_time_zone_pref_base.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/system/timezone_resolver_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "components/user_manager/user_manager.h"

namespace extensions {

namespace settings_api = api::settings_private;

namespace settings_private {

GeneratedTimeZonePrefBase::GeneratedTimeZonePrefBase(
    const std::string& pref_name,
    Profile* profile)
    : pref_name_(pref_name), profile_(profile) {
  g_browser_process->platform_part()->GetTimezoneResolverManager()->AddObserver(
      this);
}

GeneratedTimeZonePrefBase::~GeneratedTimeZonePrefBase() {
  g_browser_process->platform_part()
      ->GetTimezoneResolverManager()
      ->RemoveObserver(this);
}

void GeneratedTimeZonePrefBase::OnTimeZoneResolverUpdated() {
  NotifyObservers(pref_name_);
}

void GeneratedTimeZonePrefBase::UpdateTimeZonePrefControlledBy(
    settings_api::PrefObject* out_pref) const {
  if (chromeos::system::TimeZoneResolverManager::
          IsTimeZoneResolutionPolicyControlled()) {
    out_pref->controlled_by = settings_api::CONTROLLED_BY_DEVICE_POLICY;
    out_pref->enforcement = settings_api::ENFORCEMENT_ENFORCED;
  } else if (!profile_->IsSameProfile(
                 ProfileManager::GetPrimaryUserProfile())) {
    out_pref->controlled_by = settings_api::CONTROLLED_BY_PRIMARY_USER;
    out_pref->controlled_by_name = std::make_unique<std::string>(
        user_manager::UserManager::Get()->GetPrimaryUser()->GetDisplayEmail());
    out_pref->enforcement = settings_api::ENFORCEMENT_ENFORCED;
  }
  // If the time zone resolution settings are not policy controlled, and
  // |profile_| is the same with the primary user's profile, do nothing.
}

}  // namespace settings_private
}  // namespace extensions
