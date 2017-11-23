// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/account_consistency_manager.h"

#include <memory>

#include "build/buildflag.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/testing_pref_store.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/browser/signin_features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Checks that Dice migration happens when the reconcilor is created.
TEST(AccountConsistencyManagerTest, MigrateAtCreation) {
  content::TestBrowserThreadBundle test_thread_bundle;
  TestingProfile::Builder profile_builder;
  {
    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    chrome::RegisterUserProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  ASSERT_FALSE(profile->IsNewProfile());
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
  content::TestBrowserThreadBundle test_thread_bundle;
  signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
  TestingProfile::Builder profile_builder;
  {
    TestingPrefStore* user_prefs = new TestingPrefStore();

    // Set the read error so that Profile::IsNewProfile() returns true.
    user_prefs->set_read_error(PersistentPrefStore::PREF_READ_ERROR_NO_FILE);

    std::unique_ptr<sync_preferences::TestingPrefServiceSyncable> pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>(
            new TestingPrefStore(), new TestingPrefStore(), user_prefs,
            new TestingPrefStore(), new user_prefs::PrefRegistrySyncable(),
            new PrefNotifierImpl());
    chrome::RegisterUserProfilePrefs(pref_service->registry());
    profile_builder.SetPrefService(std::move(pref_service));
  }
  std::unique_ptr<TestingProfile> profile = profile_builder.Build();
  ASSERT_TRUE(profile->IsNewProfile());
  EXPECT_TRUE(signin::IsDiceEnabledForProfile(profile->GetPrefs()));
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)
