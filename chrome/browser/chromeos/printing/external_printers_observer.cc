// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers_observer.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "chrome/browser/chromeos/printing/external_printers.h"
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

}  // namespace

ExternalPrintersObserver::ExternalPrintersObserver(
    const ExternalPrinterPolicies& policies,
    ExternalPrinters* printers,
    Profile* profile)
    : profile_(profile), printers_(printers), policies_(policies) {
  LOG(WARNING) << "Constructing Observer";
  pref_change_registrar_.Init(profile_->GetPrefs());

  pref_change_registrar_.Add(
      policies_.access_mode,
      base::BindRepeated(&ExternalPrintersObserver::AccessModeUpdated,
                         base::Unretained(this)));
  pref_change_registrar_.Add(
      policies_.blacklist,
      base::BindRepeated(&ExternalPrintersObserver::BlacklistUpdated,
                         base::Unretained(this)));
  pref_change_registrar_.Add(
      policies_.whitelist,
      base::BindRepeated(&ExternalPrintersObserver::WhitelistUpdated,
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
  printers_->SetAccessMode(mode);
}

void ExternalPrintersObserver::BlacklistUpdated() {
  const PrefService* prefs = profile_->GetPrefs();

  std::vector<std::string> blacklist;
  const base::ListValue* list = prefs->GetList(policies_.blacklist);
  for (const base::Value& value : *list) {
    if (value.is_string()) {
      blacklist.push_back(value.GetString());
    }
  }
  printers_->SetBlacklist(blacklist);
}

void ExternalPrintersObserver::WhitelistUpdated() {
  const PrefService* prefs = profile_->GetPrefs();

  std::vector<std::string> whitelist;
  const base::ListValue* list = prefs->GetList(policies_.blacklist);
  for (const base::Value& value : *list) {
    if (value.is_string()) {
      whitelist.push_back(value.GetString());
    }
  }
  printers_->SetWhitelist(whitelist);
}

}  // namespace chromeos
