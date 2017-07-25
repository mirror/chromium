// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/common/profile_management_switches.h"

#include "base/macros.h"
#include "components/signin/core/browser/scoped_account_consistency.h"
#include "components/signin/core/common/signin_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace signin {

#if BUILDFLAG(ENABLE_MIRROR)
TEST(ProfileManagementSwitchesTest, GetAccountConsistencyMethodMirror) {
  // Mirror is enabled by default on some platforms.
  EXPECT_EQ(AccountConsistencyMethod::kMirror, GetAccountConsistencyMethod());
}
#else
TEST(ProfileManagementSwitchesTest, GetAccountConsistencyMethod) {
  // By default account consistency is disabled.
  EXPECT_EQ(AccountConsistencyMethod::kDisabled, GetAccountConsistencyMethod());

  // Check that feature flags work.
  AccountConsistencyMethod methods[] = {
    AccountConsistencyMethod::kDisabled,
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
    AccountConsistencyMethod::kDiceFixAuthErrors,
    AccountConsistencyMethod::kDice,
#endif
    AccountConsistencyMethod::kMirror
  };

  for (AccountConsistencyMethod method : methods) {
    ScopedAccountConsistency scoped_method(method);
    EXPECT_EQ(method, GetAccountConsistencyMethod());
  }
}
#endif  // BUILDFLAG(ENABLE_MIRROR)

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
// Check that the dice levels are ordered correctly.
TEST(ProfileManagementSwitchesTest, DiceLevelGE) {
  AccountConsistencyMethod methods[]{
      AccountConsistencyMethod::kDisabled,
      AccountConsistencyMethod::kDiceFixAuthErrors,
      AccountConsistencyMethod::kDice};

  for (size_t i = 0; i < arraysize(methods); ++i) {
    ScopedAccountConsistency scoped_method(methods[i]);

    // Check lesser methods.
    for (size_t j = 0; j <= i; ++j) {
      AccountConsistencyMethod lesser_method = methods[j];
      if (lesser_method == AccountConsistencyMethod::kDisabled)
        continue;  // It is invalid to call DiceLeveGE on kDisabled.
      EXPECT_TRUE(DiceLevelGE(lesser_method));
    }

    // Check greater methods.
    for (size_t j = i + 1; j < arraysize(methods); ++j) {
      AccountConsistencyMethod greater_method = methods[j];
      EXPECT_FALSE(DiceLevelGE(greater_method));
    }
  }
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

}  // namespace signin
