// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These are functions to access various profile-management flags but with
// possible overrides from Experiements.  This is done inside chrome/common
// because it is accessed by files through the chrome/ directory tree.

#ifndef COMPONENTS_SIGNIN_CORE_COMMON_PROFILE_MANAGEMENT_SWITCHES_H_
#define COMPONENTS_SIGNIN_CORE_COMMON_PROFILE_MANAGEMENT_SWITCHES_H_

#include "base/feature_list.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace signin {

// Account consistency feature. Only used on platforms where Mirror is not
// always enabled (ENABLE_MIRROR is false).
extern const base::Feature kAccountConsistencyFeature;

// The account consistency method parameter name.
extern const char kAccountConsistencyFeatureMethodParameter[];

// Account consistency method values.
extern const char kAccountConsistencyFeatureMethodMirror[];
extern const char kAccountConsistencyFeatureMethodDiceFixAuthErrors[];
extern const char kAccountConsistencyFeatureMethodDiceMigration[];
extern const char kAccountConsistencyFeatureMethodDice[];

enum class AccountConsistencyMethod {
  // No account consistency.
  kDisabled,

  // Account management UI in the avatar bubble.
  kMirror,

  // No account consistency, but Dice fixes authentication errors.
  kDiceFixAuthErrors,

  // Account management UI on Gaia webpages is enabled once the accounts become
  // consistent.
  kDiceMigration,

  // Account management UI on Gaia webpages is enabled. If accounts are not
  // consistent when this is enabled, the account reconcilor enforces the
  // consistency.
  kDice
};

void RegisterAccountConsistentyProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry);

// Returns the account consistency method.
AccountConsistencyMethod GetAccountConsistencyMethod();

// Checks whether Mirror account consistency is enabled. If enabled, the account
// management UI is available in the avatar bubble.
bool IsAccountConsistencyMirrorEnabled();

// Returns true if Dice account consistency is enabled or if the Dice migration
// process is in progress (account consistency method is kDice or
// kDiceMigration).
// To check wether Dice is enabled (i.e. the migration is complete), use
// IsAccountConsistencyEnabledForProfile().
// WARNING: returns false when the method is kDiceFixAuthErrors.
bool IsAccountConsistencyDiceAvailable();

// If true, then account management UI is done through Gaia webpages.
bool IsAccountConsistencyDiceEnabledForProfile(PrefService* user_prefs);

// Called to migrate a profile to Dice. After this call, it is enabled forever.
void MigrateProfileToDice(PrefService* user_prefs);

// Returns true if the account consistency method is kDiceFixAuthErrors,
// kDiceMigration or kDice.
bool IsDiceFixAuthErrorsEnabled();

// Whether the chrome.identity API should be multi-account.
bool IsExtensionsMultiAccount();

}  // namespace signin

#endif  // COMPONENTS_SIGNIN_CORE_COMMON_PROFILE_MANAGEMENT_SWITCHES_H_
