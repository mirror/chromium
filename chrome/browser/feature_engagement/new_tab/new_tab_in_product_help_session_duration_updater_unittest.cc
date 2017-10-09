// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/session_duration_updater.h"

#include "chrome/browser/feature_engagement/new_tab/new_tab_in_product_help_session_duration_updater.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

class NewTabInProductHelpSessionDurationUpdaterTest : public testing::Test {
 public:
  NewTabInProductHelpSessionDurationUpdaterTest() = default;
  ~NewTabInProductHelpSessionDurationUpdaterTest() override = default;

  // testing::Test:
  void SetUp() override {
    metrics::DesktopSessionDurationTracker::Initialize();

    session_duration_updater =
        new NewTabInProductHelpSessionDurationUpdater(&pref_service);
    NewTabInProductHelpSessionDurationUpdater::RegisterProfilePrefs(
        pref_service.registry());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::CleanupForTesting();
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable pref_service;
  NewTabInProductHelpSessionDurationUpdater* session_duration_updater;

  DISALLOW_COPY_AND_ASSIGN(NewTabInProductHelpSessionDurationUpdaterTest);
};

}  // namespace

// kNewTabInProductHelpObservedSessionTime should be 0 on initalization and 50
// after simulation.
TEST_F(NewTabInProductHelpSessionDurationUpdaterTest, RegisterProfilePrefs) {
  // Tests the pref is registered to 0 before any session time passes.
  EXPECT_EQ(
      0, pref_service.GetInt64(prefs::kNewTabInProductHelpObservedSessionTime));

  // Tests 50 seconds passing with an observer added.
  session_duration_updater->OnSessionEnded(base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, pref_service.GetInt64(
                    prefs::kNewTabInProductHelpObservedSessionTime));
}

// Active session time should be 0 on initalization and 50 after simulation.
TEST_F(NewTabInProductHelpSessionDurationUpdaterTest, OnSessionEnded) {
  // Tests the pref is registered to 0 before any session time passes.
  EXPECT_EQ(0, session_duration_updater
                   ->GetActiveSessionElapsedTime(
                       prefs::kNewTabInProductHelpObservedSessionTime)
                   .InSeconds());

  // Tests 50 seconds passing with an observer added.
  session_duration_updater->OnSessionEnded(base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, session_duration_updater
                    ->GetActiveSessionElapsedTime(
                        prefs::kNewTabInProductHelpObservedSessionTime)
                    .InSeconds());
}

}  // namespace feature_engagement
