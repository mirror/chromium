// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/chromeos_resolve_time_zone_by_geolocation_method_short.h"

#include <memory>

#include "base/callback.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/system/timezone_resolver_manager.h"
#include "chrome/browser/extensions/api/settings_private/chromeos_time_zone_manager_observer.h"
#include "chrome/browser/extensions/api/settings_private/generated_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"

namespace extensions {

namespace settings_private = api::settings_private;

namespace {

class GeneratedResolveTimezoneByGeolocationMetodShort
    : public GeneratedPrefTimeZoneManagerObserver {
 public:
  explicit GeneratedResolveTimezoneByGeolocationMetodShort(Profile* profile);
  ~GeneratedResolveTimezoneByGeolocationMetodShort() override;

  // GeneratedPrefsChromeOSImpl implementation:
  std::unique_ptr<settings_private::PrefObject> GetPrefObject() const override;
  SetPrefResult SetPref(const base::Value* value) override;

 private:
  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(GeneratedResolveTimezoneByGeolocationMetodShort);
};

GeneratedResolveTimezoneByGeolocationMetodShort::
    GeneratedResolveTimezoneByGeolocationMetodShort(Profile* profile)
    : GeneratedPrefTimeZoneManagerObserver(
          kResolveTimezoneByGeolocationMetodShort),
      profile_(profile) {}

GeneratedResolveTimezoneByGeolocationMetodShort::
    ~GeneratedResolveTimezoneByGeolocationMetodShort() {}

std::unique_ptr<settings_private::PrefObject>
GeneratedResolveTimezoneByGeolocationMetodShort::GetPrefObject() const {
  std::unique_ptr<settings_private::PrefObject> pref_object(
      new settings_private::PrefObject);

  pref_object->key = pref_name_;
  pref_object->type = settings_private::PREF_TYPE_NUMBER;
  pref_object->value = std::make_unique<base::Value>(static_cast<int>(
      g_browser_process->platform_part()
          ->GetTimezoneResolverManager()
          ->GetEffectiveUserTimeZoneResolveMethod(profile_->GetPrefs(), true)));

  if (chromeos::system::TimeZoneResolverManager::
          IsTimeZoneResolvePolicyControlled()) {
    pref_object->controlled_by = settings_private::CONTROLLED_BY_DEVICE_POLICY;
    pref_object->enforcement = settings_private::ENFORCEMENT_ENFORCED;
  } else if (!profile_->IsSameProfile(
                 ProfileManager::GetPrimaryUserProfile())) {
    pref_object->controlled_by = settings_private::CONTROLLED_BY_PRIMARY_USER;
    pref_object->controlled_by_name = std::make_unique<std::string>(
        user_manager::UserManager::Get()->GetPrimaryUser()->GetDisplayEmail());
    pref_object->enforcement = settings_private::ENFORCEMENT_ENFORCED;
  }

  return pref_object;
}

SetPrefResult GeneratedResolveTimezoneByGeolocationMetodShort::SetPref(
    const base::Value* value) {
  if (!value->is_int())
    return PREF_TYPE_MISMATCH;

  // Check if preference is policy or primary-user controlled.
  if (chromeos::system::TimeZoneResolverManager::
          IsTimeZoneResolvePolicyControlled() ||
      !profile_->IsSameProfile(ProfileManager::GetPrimaryUserProfile())) {
    return PREF_NOT_MODIFIABLE;
  }

  // Check if automatic time zone detection is disabled.
  // (kResolveTimezoneByGeolocationOnOff must be modified first.)
  if (!g_browser_process->platform_part()
           ->GetTimezoneResolverManager()
           ->TimeZoneResolverShouldBeRunning()) {
    return PREF_NOT_MODIFIABLE;
  }

  // In JS all numbers are doubles.
  double double_value;
  if (!value->GetAsDouble(&double_value))
    return PREF_TYPE_MISMATCH;

  const chromeos::system::TimeZoneResolverManager::TimeZoneResolveMethod
      new_value = chromeos::system::TimeZoneResolverManager::
          TimeZoneResolveMethodFromInt(static_cast<int>(double_value));
  const chromeos::system::TimeZoneResolverManager::TimeZoneResolveMethod
      current_value = g_browser_process->platform_part()
                          ->GetTimezoneResolverManager()
                          ->GetEffectiveUserTimeZoneResolveMethod(
                              profile_->GetPrefs(), true);
  if (new_value == current_value)
    return SUCCESS;

  profile_->GetPrefs()->SetInteger(::prefs::kResolveTimezoneByGeolocationMethod,
                                   static_cast<int>(new_value));

  return SUCCESS;
}

}  // anonymous namespace

const char kResolveTimezoneByGeolocationMetodShort[] =
    "generated.resolve_timezone_by_geolocation_method_short";

std::unique_ptr<GeneratedPrefImplBase>
CreateGeneratedResolveTimezoneByGeolocationMetodShort(Profile* profile) {
  return std::make_unique<GeneratedResolveTimezoneByGeolocationMetodShort>(
      profile);
}

}  // namespace extensions
