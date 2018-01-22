// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/force_restart_after_update_pref.h"

#include "chrome/common/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(GetForceRestartAfterUpdatePref, TestAll) {
  TestingPrefServiceSimple pref_service;

  RegisterForceRestartAfterUpdatePref(pref_service.registry());
  ASSERT_NE(pref_service.FindPreference(prefs::kForceRestartAfterUpdate),
            nullptr);

  EXPECT_EQ(GetForceRestartAfterUpdatePref(&pref_service),
            ForceRestartAfterUpdatePref::kUnset);

  static constexpr std::pair<int, ForceRestartAfterUpdatePref> kData[] = {
      {-1, ForceRestartAfterUpdatePref::kUnset},
      {0, ForceRestartAfterUpdatePref::kUnset},
      {1, ForceRestartAfterUpdatePref::kRestartWhenIdle},
      {2, ForceRestartAfterUpdatePref::kDeferrableRestart},
      {3, ForceRestartAfterUpdatePref::kForcedRestart},
      {4, ForceRestartAfterUpdatePref::kUnset}};
  for (const auto& data : kData) {
    pref_service.SetInteger(prefs::kForceRestartAfterUpdate, data.first);
    EXPECT_EQ(GetForceRestartAfterUpdatePref(&pref_service), data.second);
  }
}
