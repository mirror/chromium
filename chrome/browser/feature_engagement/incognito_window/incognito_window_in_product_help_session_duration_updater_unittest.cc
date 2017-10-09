// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/session_duration_updater.h"

#include "chrome/browser/feature_engagement/incognito_window/incognito_window_in_product_help_session_duration_updater.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

class IncognitoWindowInProductHelpSessionDurationUpdaterTest
    : public testing::Test {
 public:
  IncognitoWindowInProductHelpSessionDurationUpdaterTest() = default;
  ~IncognitoWindowInProductHelpSessionDurationUpdaterTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();

    session_duration_updater =
        new IncognitoWindowInProductHelpSessionDurationUpdater(&pref_service);
    IncognitoWindowInProductHelpSessionDurationUpdater::RegisterProfilePrefs(
        pref_service.registry());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::CleanupForTesting();
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable pref_service;
  IncognitoWindowInProductHelpSessionDurationUpdater* session_duration_updater;

  DISALLOW_COPY_AND_ASSIGN(
      IncognitoWindowInProductHelpSessionDurationUpdaterTest);
};

}  // namespace

// kIncognitoWindowInProductHelpObservedSessionTime should be 0 on initalization
// and 50 after simulation.
TEST_F(IncognitoWindowInProductHelpSessionDurationUpdaterTest,
       RegisterProfilePrefs) {
  // Tests the pref is registered to 0 before any session time passes.
  EXPECT_EQ(0, pref_service.GetInt64(
                   prefs::kIncognitoWindowInProductHelpObservedSessionTime));

  // Tests 50 minutes passing with an observer added.
  session_duration_updater->OnSessionEnded(base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, pref_service.GetInt64(
                    prefs::kIncognitoWindowInProductHelpObservedSessionTime));
}

// Active session time should be 0 on initalization and 50 after simulation.
TEST_F(IncognitoWindowInProductHelpSessionDurationUpdaterTest, OnSessionEnded) {
  // Tests the pref is registered to 0 before any session time passes.
  EXPECT_EQ(0, session_duration_updater
                   ->GetActiveSessionElapsedTime(
                       prefs::kIncognitoWindowInProductHelpObservedSessionTime)
                   .InSeconds());

  // Tests 50 minutes passing with an observer added.
  session_duration_updater->OnSessionEnded(base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, session_duration_updater
                    ->GetActiveSessionElapsedTime(
                        prefs::kIncognitoWindowInProductHelpObservedSessionTime)
                    .InSeconds());
}

}  // namespace feature_engagement
