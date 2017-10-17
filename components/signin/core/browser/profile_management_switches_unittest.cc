// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/common/profile_management_switches.h"

#include "base/macros.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/common/signin_features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace signin {

#if BUILDFLAG(ENABLE_MIRROR)
TEST(ProfileManagementSwitchesTest, GetAccountConsistencyMethodMirror) {
  // Mirror is enabled by default on some platforms.
  EXPECT_EQ(AccountConsistencyMethod::kMirror, GetAccountConsistencyMethod());
  EXPECT_TRUE(IsAccountConsistencyMirrorEnabled());
  EXPECT_FALSE(IsAccountConsistencyDiceAvailable());
  EXPECT_FALSE(IsDiceFixAuthErrorsEnabled());
}
#else
TEST(ProfileManagementSwitchesTest, GetAccountConsistencyMethod) {
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::RegisterAccountConsistentyProfilePrefs(pref_service.registry());

  // By default account consistency is disabled.
  EXPECT_EQ(AccountConsistencyMethod::kDisabled, GetAccountConsistencyMethod());

  struct TestCase {
    AccountConsistencyMethod method;
    bool expect_mirror_enabled;
    bool expect_dice_fix_auth_errors;
    bool expect_dice_available;
    bool expect_dice_enabled;
  };

  TestCase test_cases[] = {
    {AccountConsistencyMethod::kDisabled, false, false, false, false},
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
    {AccountConsistencyMethod::kDiceFixAuthErrors, false, true, false, false},
    {AccountConsistencyMethod::kDiceMigration, false, true, true, false},
    {AccountConsistencyMethod::kDice, false, true, true, true},
#endif
    {AccountConsistencyMethod::kMirror, true, false, false}
  };

  for (TestCase test_case : test_cases) {
    ScopedAccountConsistency scoped_method(test_case.method);
    EXPECT_EQ(test_case.method, GetAccountConsistencyMethod());
    EXPECT_EQ(test_case.expect_mirror_enabled,
              IsAccountConsistencyMirrorEnabled());
    EXPECT_EQ(test_case.expect_dice_fix_auth_errors,
              IsDiceFixAuthErrorsEnabled());
    EXPECT_EQ(test_case.expect_dice_available,
              IsAccountConsistencyDiceAvailable());
    EXPECT_EQ(test_case.expect_dice_enabled,
              IsAccountConsistencyDiceEnabledForProfile(&pref_service));
  }
}

TEST(ProfileManagementSwitchesTest, DiceMigration) {
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::RegisterAccountConsistentyProfilePrefs(pref_service.registry());

  {
    ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    MigrateProfileToDice(&pref_service);
  }

  struct TestCase {
    AccountConsistencyMethod method;
    bool expect_dice_enabled;
  };

  TestCase test_cases[] = {
    {AccountConsistencyMethod::kDisabled, false},
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
    {AccountConsistencyMethod::kDiceFixAuthErrors, false},
    {AccountConsistencyMethod::kDiceMigration, true},
    {AccountConsistencyMethod::kDice, true},
#endif
    {AccountConsistencyMethod::kMirror, false}
  };

  for (TestCase test_case : test_cases) {
    ScopedAccountConsistency scoped_method(test_case.method);
    EXPECT_EQ(test_case.expect_dice_enabled,
              IsAccountConsistencyDiceEnabledForProfile(&pref_service));
  }
}
#endif  // BUILDFLAG(ENABLE_MIRROR)

}  // namespace signin
