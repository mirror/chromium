// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_consistency_manager.h"

#include <memory>

#include "chrome/test/base/testing_profile.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

// Checks that Dice migration happens when the reconcilor is created.
TEST(AccountConsistencyManagerTest, MigrateAtCreation) {
  TestingProfile::Builder profile_builder;
  {
    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    AccountConsistencyManager::RegisterProfilePrefs(pref_service->registry());
    signin::RegisterAccountConsistencyProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  PrefService* prefs = profile->GetPrefs();

  {
    // Migration does not happen if SetDiceMigrationOnStartup() is not called.
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
    AccountConsistencyManager manager(profile.get());
    EXPECT_FALSE(manager.IsReadyForDiceMigration());
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
  }

  AccountConsistencyManager::SetDiceMigrationOnStartup(prefs, true);

  {
    // Migration does not happen if Dice is not enabled.
    signin::ScopedAccountConsistencyDiceFixAuthErrors scoped_dice_fix_errors;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
    AccountConsistencyManager manager(profile.get());
    EXPECT_TRUE(manager.IsReadyForDiceMigration());
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
  }

  {
    // Migration happens.
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
    AccountConsistencyManager manager(profile.get());
    EXPECT_TRUE(manager.IsReadyForDiceMigration());
    EXPECT_TRUE(signin::IsDiceEnabledForProfile(prefs));
  }
}

// Checks that new profiles are migrated at creation.
TEST(AccountConsistencyManagerTest, NewProfile) {
  signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
  TestingProfile::Builder profile_builder;
  {
    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    AccountConsistencyManager::RegisterProfilePrefs(pref_service->registry());
    signin::RegisterAccountConsistencyProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  PrefService* prefs = profile->GetPrefs();

  EXPECT_FALSE(signin::IsDiceEnabledForProfile(prefs));
  AccountConsistencyManager manager(profile.get());
  EXPECT_TRUE(signin::IsDiceEnabledForProfile(prefs));
}
