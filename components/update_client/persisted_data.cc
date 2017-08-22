// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/persisted_data.h"

#include <string>
#include <vector>

#include "base/guid.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/pref_names.h"
#endif

namespace update_client {

namespace {

// The following constants are copied from
// /extensions/browser/extension_prefs.cc.
const char kActiveBit[] = "active_bit";
const char kLastRollCallDay[] = "lastpingday";
const char kLastActivePingDay[] = "last_active_pingday";

// Unknown date values.
const int kDateLastRollCallUnknown = -2;
const int kDateLastActiveUnknown = -2;

#if !BUILDFLAG(ENABLE_EXTENSIONS)
// This constant has the exact same value as
// |extensions::pref_names::kUpdateClientData| declared/defined in
// /extensions/browser/pref_names.h. Since pref_names.h is only available when
// extensions module is built, we have to redeclare this constant here.
const char kUpdateClientData[] = "updateclientdata";
#endif

const char* UpdateClientDataName() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return extensions::pref_names::kUpdateClientData;
#else
  return kUpdateClientData;
#endif
}

#if BUILDFLAG(ENABLE_EXTENSIONS)

const base::DictionaryValue* GetExtensionPref(PrefService* pref_service,
                                              const std::string& id) {
  if (!pref_service) {
    return nullptr;
  }
  const base::DictionaryValue* extensions =
      pref_service->GetDictionary(extensions::pref_names::kExtensions);
  const base::DictionaryValue* extension_dict = nullptr;
  if (!extensions || !extensions->GetDictionary(id, &extension_dict)) {
    return nullptr;
  }
  return extension_dict;
}

// Deserializes a 64bit integer stored as a string value.
bool ReadInt64(const base::DictionaryValue* dictionary,
               const char* key,
               int64_t* value) {
  if (!dictionary) {
    return false;
  }

  std::string string_value;
  if (!dictionary->GetString(key, &string_value)) {
    return false;
  }

  return base::StringToInt64(string_value, value);
}

base::Time ReadTime(const base::DictionaryValue* dictionary, const char* key) {
  int64_t value;
  if (ReadInt64(dictionary, key, &value)) {
    return base::Time() + base::TimeDelta::FromMicroseconds(value);
  }
  return base::Time();
}

int CalcDaysSinceLast(const base::DictionaryValue* dict, const char* key) {
  base::Time time = ReadTime(dict, key);
  if (time.is_null()) {
    return -2;
  }
  int days = (base::Time::Now() - time).InDays();
  return (days < 0 ? 0 : days);
}

#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

}  // namespace

PersistedData::PersistedData(PrefService* pref_service)
    : pref_service_(pref_service) {}

PersistedData::~PersistedData() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

int PersistedData::GetInt(const std::string& id,
                          const std::string& key,
                          int fallback) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We assume ids do not contain '.' characters.
  DCHECK_EQ(std::string::npos, id.find('.'));
  if (!pref_service_)
    return fallback;
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(UpdateClientDataName());
  if (!dict)
    return fallback;
  int result = 0;
  return dict->GetInteger(
             base::StringPrintf("apps.%s.%s", id.c_str(), key.c_str()), &result)
             ? result
             : fallback;
}

std::string PersistedData::GetString(const std::string& id,
                                     const std::string& key) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // We assume ids do not contain '.' characters.
  DCHECK_EQ(std::string::npos, id.find('.'));
  if (!pref_service_)
    return std::string();
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(UpdateClientDataName());
  if (!dict)
    return std::string();
  std::string result;
  return dict->GetString(
             base::StringPrintf("apps.%s.%s", id.c_str(), key.c_str()), &result)
             ? result
             : std::string();
}

int PersistedData::GetDateLastRollCall(const std::string& id) const {
  return GetInt(id, "dlrc", kDateLastRollCallUnknown);
}

std::string PersistedData::GetPingFreshness(const std::string& id) const {
  std::string result = GetString(id, "pf");
  return !result.empty() ? base::StringPrintf("{%s}", result.c_str()) : result;
}

std::string PersistedData::GetCohort(const std::string& id) const {
  return GetString(id, "cohort");
}

std::string PersistedData::GetCohortName(const std::string& id) const {
  return GetString(id, "cohortname");
}

std::string PersistedData::GetCohortHint(const std::string& id) const {
  return GetString(id, "cohorthint");
}

void PersistedData::SetDateLastRollCall(const std::vector<std::string>& ids,
                                        int datenum) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_ || datenum < 0)
    return;
  DictionaryPrefUpdate update(pref_service_, UpdateClientDataName());
  for (auto id : ids) {
    // We assume ids do not contain '.' characters.
    DCHECK_EQ(std::string::npos, id.find('.'));
    update->SetInteger(base::StringPrintf("apps.%s.dlrc", id.c_str()), datenum);
    update->SetString(base::StringPrintf("apps.%s.pf", id.c_str()),
                      base::GenerateGUID());
  }
}

void PersistedData::SetString(const std::string& id,
                              const std::string& key,
                              const std::string& value) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_)
    return;
  DictionaryPrefUpdate update(pref_service_, UpdateClientDataName());
  update->SetString(base::StringPrintf("apps.%s.%s", id.c_str(), key.c_str()),
                    value);
}

void PersistedData::SetCohort(const std::string& id,
                              const std::string& cohort) {
  SetString(id, "cohort", cohort);
}

void PersistedData::SetCohortName(const std::string& id,
                                  const std::string& cohort_name) {
  SetString(id, "cohortname", cohort_name);
}

void PersistedData::SetCohortHint(const std::string& id,
                                  const std::string& cohort_hint) {
  SetString(id, "cohorthint", cohort_hint);
}

void PersistedData::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(UpdateClientDataName());
#if BUILDFLAG(ENABLE_EXTENSIONS)
  registry->RegisterDictionaryPref(extensions::pref_names::kExtensions);
#endif
}

#if BUILDFLAG(ENABLE_EXTENSIONS)

int PersistedData::GetDaysSinceLastRollCall(const std::string& id) const {
  return CalcDaysSinceLast(GetExtensionPref(pref_service_, id),
                           kLastRollCallDay);
}

int PersistedData::GetDateLastActive(const std::string& id) const {
  return GetInt(id, "dla", kDateLastActiveUnknown);
}

int PersistedData::GetDaysSinceLastActive(const std::string& id) const {
  return CalcDaysSinceLast(GetExtensionPref(pref_service_, id),
                           kLastActivePingDay);
}

bool PersistedData::GetActiveBit(const std::string& id) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  const base::DictionaryValue* extension_dict =
      GetExtensionPref(pref_service_, id);
  bool result = false;
  if (extension_dict && extension_dict->GetBoolean(kActiveBit, &result)) {
    return result;
  }
  return false;
}

void PersistedData::SetDateLastActive(const std::vector<std::string>& ids,
                                      int datenum) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_ || datenum < 0) {
    return;
  }
  DictionaryPrefUpdate update(pref_service_, UpdateClientDataName());
  for (auto id : ids) {
    // We assume ids do not contain '.' characters.
    DCHECK_EQ(std::string::npos, id.find('.'));
    if (GetActiveBit(id)) {
      update->SetInteger(base::StringPrintf("apps.%s.dla", id.c_str()),
                         datenum);
    }
  }
}

void PersistedData::ClearActiveBit(const std::vector<std::string>& ids) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_) {
    return;
  }
  prefs::ScopedDictionaryPrefUpdate update(pref_service_,
                                           extensions::pref_names::kExtensions);
  for (auto id : ids) {
    bool value = false;
    std::string key = base::StringPrintf("%s.%s", id.c_str(), kActiveBit);
    if (update->GetBoolean(key, &value)) {
      if (value) {
        update->SetBoolean(key, false);
      }
    }
  }
}

void PersistedData::SetActiveBit(const std::string& id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!pref_service_) {
    return;
  }
  prefs::ScopedDictionaryPrefUpdate update(pref_service_,
                                           extensions::pref_names::kExtensions);
  update->SetBoolean(base::StringPrintf("%s.%s", id.c_str(), kActiveBit), true);
}

void PersistedData::SetLastRollCallTime(const std::string& id,
                                        const base::Time& time) {
  prefs::ScopedDictionaryPrefUpdate update(pref_service_,
                                           extensions::pref_names::kExtensions);
  std::string string_value =
      base::Int64ToString((time - base::Time()).InMicroseconds());
  update->SetString(base::StringPrintf("%s.%s", id.c_str(), kLastRollCallDay),
                    string_value);
}

void PersistedData::SetLastActiveTime(const std::string& id,
                                      const base::Time& time) {
  if (!GetActiveBit(id)) {
    return;
  }
  prefs::ScopedDictionaryPrefUpdate update(pref_service_,
                                           extensions::pref_names::kExtensions);
  std::string string_value =
      base::Int64ToString((time - base::Time()).InMicroseconds());
  update->SetString(base::StringPrintf("%s.%s", id.c_str(), kLastActivePingDay),
                    string_value);
}

#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

}  // namespace update_client
