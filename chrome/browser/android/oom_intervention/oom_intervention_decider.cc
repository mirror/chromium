// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/oom_intervention_decider.h"

#include "chrome/browser/profiles/profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {

const char kOomInterventionDecider[] = "oom_intervention_decider";

const char kBlacklist[] = "oom_intervention_blacklist";
const char kDeclinedHostList[] = "oom_intervention_declined_host_list";
const char kOomDetectedHostList[] = "oom_intervention_oom_detected_host_list";

}  // namespace

const size_t OomInterventionDecider::kMaxListSize = 10;
const size_t OomInterventionDecider::kMaxBlacklistSize = 6;

// static
void OomInterventionDecider::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(kBlacklist);
  registry->RegisterListPref(kDeclinedHostList);
  registry->RegisterListPref(kOomDetectedHostList);
}

// static
OomInterventionDecider* OomInterventionDecider::GetForBrowserContext(
    content::BrowserContext* context) {
  if (!context->GetUserData(kOomInterventionDecider)) {
    PrefService* prefs = Profile::FromBrowserContext(context)->GetPrefs();
    context->SetUserData(kOomInterventionDecider,
                         base::MakeUnique<OomInterventionDecider>(prefs));
  }
  return static_cast<OomInterventionDecider*>(
      context->GetUserData(kOomInterventionDecider));
}

OomInterventionDecider::OomInterventionDecider(PrefService* prefs)
    : prefs_(prefs) {}

OomInterventionDecider::~OomInterventionDecider() = default;

bool OomInterventionDecider::CanTriggerIntervention(
    const std::string& host) const {
  if (IsOptedOut(host))
    return false;

  // Check whether OOM was observed before checking declined host list in favor
  // of triggering intervention after OOM.
  if (IsInList(kOomDetectedHostList, host))
    return true;

  if (IsInList(kDeclinedHostList, host))
    return false;

  return true;
}

void OomInterventionDecider::OnInterventionDeclined(const std::string& host) {
  if (IsOptedOut(host))
    return;

  if (IsInList(kDeclinedHostList, host)) {
    AddToList(kBlacklist, host);
  } else {
    AddToList(kDeclinedHostList, host);
  }
}

void OomInterventionDecider::OnOomDetected(const std::string& host) {
  if (IsOptedOut(host))
    return;
  AddToList(kOomDetectedHostList, host);
}

void OomInterventionDecider::ClearData() {
  prefs_->ClearPref(kBlacklist);
  prefs_->ClearPref(kDeclinedHostList);
  prefs_->ClearPref(kOomDetectedHostList);
}

bool OomInterventionDecider::IsOptedOut(const std::string& host) const {
  const base::Value::ListStorage& blacklist =
      prefs_->GetList(kBlacklist)->GetList();
  if (blacklist.size() >= kMaxBlacklistSize)
    return true;

  return IsInList(kBlacklist, host);
}

bool OomInterventionDecider::IsInList(const char* list_name,
                                      const std::string& host) const {
  const base::Value::ListStorage& list = prefs_->GetList(list_name)->GetList();
  for (auto it = list.begin(); it != list.end(); ++it) {
    if (it->GetString() == host)
      return true;
  }
  return false;
}

void OomInterventionDecider::AddToList(const char* list_name,
                                       const std::string& host) {
  if (IsInList(list_name, host))
    return;
  ListPrefUpdate update(prefs_, list_name);
  base::Value::ListStorage& list = update.Get()->GetList();
  list.push_back(base::Value(host));
  if (list.size() > kMaxListSize)
    list.erase(list.begin());
}
