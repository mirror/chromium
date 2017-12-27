// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/generated_prefs.h"

#include "base/callback.h"
#include "build/build_config.h"
#include "chrome/common/extensions/api/settings_private.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/extensions/api/settings_private/chromeos_resolve_time_zone_by_geolocation_method_short.h"
#include "chrome/browser/extensions/api/settings_private/chromeos_resolve_time_zone_by_geolocation_on_off.h"
#endif

namespace extensions {

namespace settings_private = api::settings_private;

GeneratedPrefs::GeneratedPrefs(Profile* profile) {
#if defined(OS_CHROMEOS)
  prefs_[kResolveTimezoneByGeolocationOnOff] =
      CreateGeneratedResolveTimezoneByGeolocationOnOff(profile);
  prefs_[kResolveTimezoneByGeolocationMetodShort] =
      CreateGeneratedResolveTimezoneByGeolocationMetodShort(profile);
#endif
}

GeneratedPrefs::~GeneratedPrefs() {}

GeneratedPrefImplBase* GeneratedPrefs::FindPrefImpl(
    const std::string& pref_name) const {
  const PrefsMap::const_iterator it = prefs_.find(pref_name);
  if (it == prefs_.end())
    return nullptr;

  return it->second.get();
}

bool GeneratedPrefs::IsGenerated(const std::string& pref_name) const {
  return FindPrefImpl(pref_name) != nullptr;
}

std::unique_ptr<settings_private::PrefObject> GeneratedPrefs::GetPref(
    const std::string& pref_name) const {
  GeneratedPrefImplBase* impl = FindPrefImpl(pref_name);
  if (!impl)
    return nullptr;

  return impl->GetPrefObject();
}

SetPrefResult GeneratedPrefs::SetPref(const std::string& pref_name,
                                      const base::Value* value) {
  GeneratedPrefImplBase* impl = FindPrefImpl(pref_name);
  if (!impl)
    return PREF_TYPE_UNSUPPORTED;

  return impl->SetPref(value);
}

void GeneratedPrefs::AddPrefObserver(
    const std::string& pref_name,
    const GeneratedPrefs::PrefChangeObserver& observer) {
  GeneratedPrefImplBase* impl = FindPrefImpl(pref_name);
  if (!impl)
    return;

  impl->AddPrefObserver(observer);
}

GeneratedPrefImplBase::GeneratedPrefImplBase() {}
GeneratedPrefImplBase::~GeneratedPrefImplBase() {}

void GeneratedPrefImplBase::AddPrefObserver(
    const GeneratedPrefs::PrefChangeObserver& observer) {
  observers_.push_back(observer);
}

void GeneratedPrefImplBase::NotifyObservers(const std::string& pref_name) {
  for (const auto& observer : observers_)
    observer.Run(pref_name);
}

}  // namespace extensions
