// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers_observer.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "chrome/browser/chromeos/printing/external_printers.h"
#include "chrome/browser/chromeos/printing/external_printers_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace chromeos {

namespace {

// Enumeration values for NativePrintersBulkAccessMode.
constexpr int kBlacklistAccess = 0;
constexpr int kWhitelistAccess = 1;
constexpr int kAllAccess = 2;

// Extracts the list of strings named |policy_name| from |prefs| and returns it.
std::vector<std::string> FromPrefs(const PrefService* prefs,
                                   const std::string& policy_name) {
  std::vector<std::string> string_list;
  const base::ListValue* list = prefs->GetList(policy_name);
  for (const base::Value& value : *list) {
    if (value.is_string()) {
      string_list.push_back(value.GetString());
    }
  }

  return string_list;
}

}  // namespace

ExternalPrintersObserver::ExternalPrintersObserver(
    const ExternalPrinterPolicies& policies,
    Profile* profile)
    : profile_(profile), policies_(policies) {
  pref_change_registrar_.Init(profile_->GetPrefs());

  pref_change_registrar_.Add(
      policies_.access_mode,
      base::BindRepeating(&ExternalPrintersObserver::AccessModeUpdated,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      policies_.blacklist,
      base::BindRepeating(&ExternalPrintersObserver::BlacklistUpdated,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      policies_.whitelist,
      base::BindRepeating(&ExternalPrintersObserver::WhitelistUpdated,
                          base::Unretained(this)));
  Initialize();
}

void ExternalPrintersObserver::Initialize() {
  BlacklistUpdated();
  WhitelistUpdated();
  AccessModeUpdated();
}

void ExternalPrintersObserver::AccessModeUpdated() {
  const PrefService* prefs = profile_->GetPrefs();
  ExternalPrinters::AccessMode mode = ExternalPrinters::UNSET;
  switch (prefs->GetInteger(policies_.access_mode)) {
    case kBlacklistAccess:
      mode = ExternalPrinters::BLACKLIST_ONLY;
      break;
    case kWhitelistAccess:
      mode = ExternalPrinters::WHITELIST_ONLY;
      break;
    case kAllAccess:
      mode = ExternalPrinters::ALL_ACCESS;
      break;
    default:
      NOTREACHED();
      break;
  }
  auto* printers = ExternalPrintersFactory::Get()->GetForProfile(profile_);
  if (printers)
    printers->SetAccessMode(mode);
}

void ExternalPrintersObserver::BlacklistUpdated() {
  auto* printers = ExternalPrintersFactory::Get()->GetForProfile(profile_);
  if (printers)
    printers->SetBlacklist(
        FromPrefs(profile_->GetPrefs(), policies_.blacklist));
}

void ExternalPrintersObserver::WhitelistUpdated() {
  auto* printers = ExternalPrintersFactory::Get()->GetForProfile(profile_);
  if (printers)
    printers->SetWhitelist(
        FromPrefs(profile_->GetPrefs(), policies_.whitelist));
}

}  // namespace chromeos
