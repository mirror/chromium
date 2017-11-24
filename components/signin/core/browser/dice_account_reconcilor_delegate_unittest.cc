// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/dice_account_reconcilor_delegate.h"

#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// Checks that Dice migration happens when the reconcilor is created.
TEST(DiceAccountReconcilorDelegateTest, MigrateAtCreation) {
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::DiceAccountReconcilorDelegate::RegisterProfilePrefs(
      pref_service.registry());
  signin::RegisterAccountConsistencyProfilePrefs(pref_service.registry());
  FakeProfileOAuth2TokenService token_service;

  {
    // Migration does not happen if SetDiceMigrationOnStartup() is not called.
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(&pref_service));
    signin::DiceAccountReconcilorDelegate delegate(&token_service,
                                                   &pref_service, false);
    EXPECT_FALSE(delegate.IsReadyForDiceMigration(false /* is_new_profile */));
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(&pref_service));
  }

  signin::DiceAccountReconcilorDelegate::SetDiceMigrationOnStartup(
      &pref_service, true);

  {
    // Migration does not happen if Dice is not enabled.
    signin::ScopedAccountConsistencyDiceFixAuthErrors scoped_dice_fix_errors;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(&pref_service));
    signin::DiceAccountReconcilorDelegate delegate(&token_service,
                                                   &pref_service, false);
    EXPECT_TRUE(delegate.IsReadyForDiceMigration(false /* is_new_profile */));
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(&pref_service));
  }

  {
    // Migration happens.
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    EXPECT_FALSE(signin::IsDiceEnabledForProfile(&pref_service));
    signin::DiceAccountReconcilorDelegate delegate(&token_service,
                                                   &pref_service, false);
    EXPECT_TRUE(delegate.IsReadyForDiceMigration(false /* is_new_profile */));
    EXPECT_TRUE(signin::IsDiceEnabledForProfile(&pref_service));
  }
}

// Checks that new profiles are migrated at creation.
TEST(DiceAccountReconcilorDelegateTest, NewProfile) {
  signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::DiceAccountReconcilorDelegate::RegisterProfilePrefs(
      pref_service.registry());
  signin::RegisterAccountConsistencyProfilePrefs(pref_service.registry());
  FakeProfileOAuth2TokenService token_service;
  EXPECT_FALSE(signin::IsDiceEnabledForProfile(&pref_service));
  signin::DiceAccountReconcilorDelegate delegate(&token_service, &pref_service,
                                                 true);
  EXPECT_TRUE(signin::IsDiceEnabledForProfile(&pref_service));
}

TEST(DiceAccountReconcilorDelegateTest, RevokeTokens) {
  sync_preferences::TestingPrefServiceSyncable pref_service;
  signin::DiceAccountReconcilorDelegate::RegisterProfilePrefs(
      pref_service.registry());
  signin::RegisterAccountConsistencyProfilePrefs(pref_service.registry());
  FakeProfileOAuth2TokenService token_service;
  signin::DiceAccountReconcilorDelegate delegate(&token_service, &pref_service,
                                                 false);
  token_service.UpdateCredentials("main", "token");
  token_service.UpdateCredentials("secondary", "token");

  {
    // Dice is enabled, don't revoke.
    signin::ScopedAccountConsistencyDice scoped_dice;
    delegate.OnReconcileStarted(std::vector<std::string>{"main", "secondary"},
                                std::vector<gaia::ListedAccount>(), "main");
    EXPECT_TRUE(token_service.RefreshTokenIsAvailable("main"));
    EXPECT_TRUE(token_service.RefreshTokenIsAvailable("secondary"));
  }
  {
    signin::ScopedAccountConsistencyDiceMigration scoped_dice_migration;
    // Gaia accounts are not empty, don't revoke.
    gaia::ListedAccount gaia_account;
    gaia_account.id = "other";
    delegate.OnReconcileStarted(std::vector<std::string>{"main", "secondary"},
                                std::vector<gaia::ListedAccount>{gaia_account},
                                "main");
    EXPECT_TRUE(token_service.RefreshTokenIsAvailable("main"));
    EXPECT_TRUE(token_service.RefreshTokenIsAvailable("secondary"));

    // Don't revoke the primary account.
    delegate.OnReconcileStarted(std::vector<std::string>{"main", "secondary"},
                                std::vector<gaia::ListedAccount>(), "main");
    EXPECT_TRUE(token_service.RefreshTokenIsAvailable("main"));
    EXPECT_FALSE(token_service.RefreshTokenIsAvailable("secondary"));

    // No primary account, revoke everything.
    token_service.UpdateCredentials("secondary", "token");
    delegate.OnReconcileStarted(std::vector<std::string>{"main", "secondary"},
                                std::vector<gaia::ListedAccount>(), "");
    EXPECT_FALSE(token_service.RefreshTokenIsAvailable("main"));
    EXPECT_FALSE(token_service.RefreshTokenIsAvailable("secondary"));
  }
}
