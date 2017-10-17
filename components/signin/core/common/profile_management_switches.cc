// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/common/profile_management_switches.h"

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/common/signin_features.h"
#include "components/signin/core/common/signin_switches.h"

namespace {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
const char kDiceEnabledPref[] = "signin.DiceEnabled";
#endif
}

namespace signin {

// base::Feature definitions.
const base::Feature kAccountConsistencyFeature{
    "AccountConsistency", base::FEATURE_DISABLED_BY_DEFAULT};
const char kAccountConsistencyFeatureMethodParameter[] = "method";
const char kAccountConsistencyFeatureMethodMirror[] = "mirror";
const char kAccountConsistencyFeatureMethodDiceFixAuthErrors[] =
    "dice_fix_auth_errors";
const char kAccountConsistencyFeatureMethodDiceMigration[] = "dice_migration";
const char kAccountConsistencyFeatureMethodDice[] = "dice";

void RegisterAccountConsistentyProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  registry->RegisterBooleanPref(kDiceEnabledPref, false);
#endif
}

AccountConsistencyMethod GetAccountConsistencyMethod() {
#if BUILDFLAG(ENABLE_MIRROR)
  // Mirror is always enabled on Android and iOS.
  return AccountConsistencyMethod::kMirror;
#else
  if (!base::FeatureList::IsEnabled(kAccountConsistencyFeature))
    return AccountConsistencyMethod::kDisabled;

  std::string method_value = base::GetFieldTrialParamValueByFeature(
      kAccountConsistencyFeature, kAccountConsistencyFeatureMethodParameter);

  if (method_value == kAccountConsistencyFeatureMethodMirror)
    return AccountConsistencyMethod::kMirror;
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  else if (method_value == kAccountConsistencyFeatureMethodDiceFixAuthErrors)
    return AccountConsistencyMethod::kDiceFixAuthErrors;
  else if (method_value == kAccountConsistencyFeatureMethodDiceMigration)
    return AccountConsistencyMethod::kDiceMigration;
  else if (method_value == kAccountConsistencyFeatureMethodDice)
    return AccountConsistencyMethod::kDice;
#endif

  return AccountConsistencyMethod::kDisabled;
#endif  // BUILDFLAG(ENABLE_MIRROR)
}

bool IsAccountConsistencyMirrorEnabled() {
  return GetAccountConsistencyMethod() == AccountConsistencyMethod::kMirror;
}

bool IsAccountConsistencyDiceAvailable() {
  return (GetAccountConsistencyMethod() ==
          AccountConsistencyMethod::kDiceMigration) ||
         (GetAccountConsistencyMethod() == AccountConsistencyMethod::kDice);
}

bool IsAccountConsistencyDiceEnabledForProfile(PrefService* user_prefs) {
  DCHECK(user_prefs);
  switch (GetAccountConsistencyMethod()) {
    case AccountConsistencyMethod::kDisabled:
    case AccountConsistencyMethod::kMirror:
    case AccountConsistencyMethod::kDiceFixAuthErrors:
      return false;
    case AccountConsistencyMethod::kDice:
      return true;
    case AccountConsistencyMethod::kDiceMigration:
      return user_prefs->GetBoolean(kDiceEnabledPref);
  }
  NOTREACHED();
  return false;
}

void MigrateProfileToDice(PrefService* user_prefs) {
  DCHECK(IsAccountConsistencyDiceAvailable());
  user_prefs->SetBoolean(kDiceEnabledPref, true);
}

bool IsDiceFixAuthErrorsEnabled() {
  AccountConsistencyMethod method = GetAccountConsistencyMethod();
  return (method == AccountConsistencyMethod::kDiceFixAuthErrors) ||
         (method == AccountConsistencyMethod::kDiceMigration) ||
         (method == AccountConsistencyMethod::kDice);
}

bool IsExtensionsMultiAccount() {
#if defined(OS_ANDROID) || defined(OS_IOS)
  NOTREACHED() << "Extensions are not enabled on Android or iOS";
  // Account consistency is enabled on Android and iOS.
  return false;
#endif

  return base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kExtensionsMultiAccount) ||
         GetAccountConsistencyMethod() == AccountConsistencyMethod::kMirror;
}

}  // namespace signin
