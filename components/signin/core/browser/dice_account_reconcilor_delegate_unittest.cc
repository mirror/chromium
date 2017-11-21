// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/dice_account_reconcilor_delegate.h"

#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class FakeSigninClient : public TestSigninClient {
 public:
  explicit FakeSigninClient(PrefService* prefs) : TestSigninClient(prefs) {}

  bool migrated() { return migrated_; }

  // TestSigninClient:
  bool IsDiceEnabled() override { return migrated_; }
  void MigrateProfileToDice() override { migrated_ = true; }

 private:
  bool migrated_ = false;
};
}  // namespace

// Checks that Dice migration happens when the reconcilor is created.
TEST(DiceAccountReconcilorDelegateTest, MigrateAtCreation) {
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::DiceAccountReconcilorDelegate::RegisterProfilePrefs(
      pref_service.registry());
  signin::RegisterAccountConsistencyProfilePrefs(pref_service.registry());
  FakeSigninClient client(&pref_service);
  client.set_account_consistency_method(
      signin::AccountConsistencyMethod::kDiceMigration);

  {
    // Migration does not happen if SetDiceMigrationOnStartup() is not called.
    signin::DiceAccountReconcilorDelegate delegate(&client, false);
    EXPECT_FALSE(delegate.IsReadyForDiceMigration(false /* is_new_profile */));
    EXPECT_FALSE(client.migrated());
  }

  signin::DiceAccountReconcilorDelegate::SetDiceMigrationOnStartup(
      &pref_service, true);

  {
    // Migration does not happen if Dice is not enabled.
    client.set_account_consistency_method(
        signin::AccountConsistencyMethod::kDiceFixAuthErrors);
    signin::DiceAccountReconcilorDelegate delegate(&client, false);
    EXPECT_TRUE(delegate.IsReadyForDiceMigration(false /* is_new_profile */));
    EXPECT_FALSE(client.migrated());
  }

  {
    // Migration happens.
    client.set_account_consistency_method(
        signin::AccountConsistencyMethod::kDiceMigration);
    signin::DiceAccountReconcilorDelegate delegate(&client, false);
    EXPECT_TRUE(delegate.IsReadyForDiceMigration(false /* is_new_profile */));
    EXPECT_TRUE(client.migrated());
  }
}

// Checks that new profiles are migrated at creation.
TEST(DiceAccountReconcilorDelegateTest, NewProfile) {
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::DiceAccountReconcilorDelegate::RegisterProfilePrefs(
      pref_service.registry());
  FakeSigninClient client(&pref_service);
  client.set_account_consistency_method(
      signin::AccountConsistencyMethod::kDiceMigration);
  signin::RegisterAccountConsistencyProfilePrefs(pref_service.registry());
  signin::DiceAccountReconcilorDelegate delegate(&client, true);
  EXPECT_TRUE(client.migrated());
}
