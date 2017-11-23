// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_consistency_manager.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "build/buildflag.h"
#include "chrome/browser/profiles/profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_features.h"

namespace {

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Preference indicating that the Dice migration should happen at the next
// Chrome startup.
const char kDiceMigrationOnStartupPref[] =
    "signin.AccountReconcilor.kDiceMigrationOnStartup";

const char kDiceMigrationStatusHistogram[] = "Signin.DiceMigrationStatus";

// Used for UMA histogram kDiceMigrationStatusHistogram.
// Do not remove or re-order values.
enum class DiceMigrationStatus {
  kEnabled,
  kDisabledReadyForMigration,
  kDisabledNotReadyForMigration,

  // This is the last value. New values should be inserted above.
  kDiceMigrationStatusCount
};
#endif

}  // namespace

AccountConsistencyManager::AccountConsistencyManager(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  bool is_ready_for_dice = IsReadyForDiceMigration();
  PrefService* user_prefs = profile->GetPrefs();
  if (is_ready_for_dice && signin::IsDiceMigrationEnabled()) {
    if (!signin::IsDiceEnabledForProfile(user_prefs))
      VLOG(1) << "Profile is migrating to Dice";
    signin::MigrateProfileToDice(user_prefs);
    DCHECK(signin::IsDiceEnabledForProfile(user_prefs));
  }
  UMA_HISTOGRAM_ENUMERATION(
      kDiceMigrationStatusHistogram,
      signin::IsDiceEnabledForProfile(user_prefs)
          ? DiceMigrationStatus::kEnabled
          : (is_ready_for_dice
                 ? DiceMigrationStatus::kDisabledReadyForMigration
                 : DiceMigrationStatus::kDisabledNotReadyForMigration),
      DiceMigrationStatus::kDiceMigrationStatusCount);

#endif
}

AccountConsistencyManager::~AccountConsistencyManager() {}

// static
void AccountConsistencyManager::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  registry->RegisterBooleanPref(kDiceMigrationOnStartupPref, false);
#endif
}

signin::AccountConsistencyMethod
AccountConsistencyManager::GetAccountConsistencyMethod() {
  // The global account consistency method may be tweaked per profile.
  signin::AccountConsistencyMethod method =
      signin::GetAccountConsistencyMethod();

  if ((method == signin::AccountConsistencyMethod::kDiceMigration) &&
      signin::IsDiceEnabledForProfile(profile_->GetPrefs())) {
    // If the migration is complete, return kDice.
    return signin::AccountConsistencyMethod::kDice;
  }

  return method;
}

// static
signin::AccountConsistencyMethod
AccountConsistencyManager::GetAccountConsistencyMethodForProfile(
    BooleanPrefMember* dice_pref_member) {
  // TODO
  return signin::GetAccountConsistencyMethod();
}

void AccountConsistencyManager::SetReadyForDiceMigration(bool is_ready) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  SetDiceMigrationOnStartup(profile_->GetPrefs(), is_ready);
#endif
}

// static
void AccountConsistencyManager::SetDiceMigrationOnStartup(PrefService* prefs,
                                                          bool migrate) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  VLOG(1) << "Dice migration on next startup: " << migrate;
  prefs->SetBoolean(kDiceMigrationOnStartupPref, migrate);
#endif
}

bool AccountConsistencyManager::IsReadyForDiceMigration() {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  return profile_->IsNewProfile() ||
         profile_->GetPrefs()->GetBoolean(kDiceMigrationOnStartupPref);
#else
  return false;
#endif
}
