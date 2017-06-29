// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker.h"

#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using metrics::DesktopSessionDurationTracker;

namespace new_tab_feature_engagement_tracker {

class FakeNewTabFeatureEngagementTracker
    : public NewTabFeatureEngagementTracker {
 public:
  explicit FakeNewTabFeatureEngagementTracker(Profile* profile)
      : testing_profile_(profile),
        notify_new_tab_opened_(false),
        notify_omnibox_navigation_(false),
        notify_session_time_met_(false),
        promo_seen_(false) {}

  bool promo_seen() const {
    return !notify_new_tab_opened_ && notify_omnibox_navigation_ &&
           notify_session_time_met_;
  }

  // new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker
  Profile* GetProfile() override { return testing_profile_; }

  void NotifyNewTabOpened() override { notify_new_tab_opened_ = true; }

  void NotifyOmniboxNavigation() override { notify_omnibox_navigation_ = true; }

  void NotifySessionTimeMet() override { notify_session_time_met_ = true; }

  bool HasEnoughSessionTimeElapsed() override { return true; }

  void OnSessionEnded(base::TimeDelta delta) override {
    NewTabFeatureEngagementTracker::OnSessionEnded(delta);
  }

 private:
  bool promo_seen_;
  bool notify_new_tab_opened_;
  bool notify_omnibox_navigation_;
  bool notify_session_time_met_;
  Profile* testing_profile_;

  DISALLOW_COPY_AND_ASSIGN(FakeNewTabFeatureEngagementTracker);
};

class FakeFeatureEngagementTracker : public FeatureEngagementTracker {
 public:
  FakeFeatureEngagementTracker() {}

  // feature_engagement_tracker::FeatureEngagementTracker
  bool ShouldTriggerHelpUI(const base::Feature& feature) override {
    return true;
  }
};

class NewTabFeatureEngagementTrackerTest : public testing::Test {
 public:
  NewTabFeatureEngagementTrackerTest()
      : manager_(TestingBrowserProcess::GetGlobal()) {}
  ~NewTabFeatureEngagementTrackerTest() override {}

  // testing::Test:
  void SetUp() override { ASSERT_TRUE(manager_.SetUp()); }

  TestingProfileManager* manager() { return &manager_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager manager_;

  DISALLOW_COPY_AND_ASSIGN(NewTabFeatureEngagementTrackerTest);
};

TEST_F(NewTabFeatureEngagementTrackerTest, TestNotifyNewTabOpened) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_1");
  ASSERT_TRUE(profile);

  DesktopSessionDurationTracker::Initialize();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(profile);

  instance->NotifyNewTabOpened();

  // If a new tab is created, then the educational promo
  // should not be seen.
  EXPECT_FALSE(instance->promo_seen());

  DesktopSessionDurationTracker::Get()->RemoveObserver(instance.release());
  manager()->DeleteTestingProfile("profile_1");
}

TEST_F(NewTabFeatureEngagementTrackerTest, TestOmniboxNavigation) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_2");
  ASSERT_TRUE(profile);

  DesktopSessionDurationTracker::Initialize();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(profile);

  EXPECT_FALSE(instance->promo_seen());

  // Two active hours have passed.
  instance->NotifySessionTimeMet();

  instance->NotifyOmniboxNavigation();

  // If two hours have passed and the user has navigated in the omnibox,
  // then the educational promo should be seen.
  EXPECT_TRUE(instance->promo_seen());

  DesktopSessionDurationTracker::Get()->RemoveObserver(instance.release());
  manager()->DeleteTestingProfile("profile_2");
}

TEST_F(NewTabFeatureEngagementTrackerTest, TestNotifySessionTimeMet) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_3");
  ASSERT_TRUE(profile);

  DesktopSessionDurationTracker::Initialize();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(profile);

  EXPECT_FALSE(instance->promo_seen());

  instance->NotifyOmniboxNavigation();
  instance->NotifySessionTimeMet();

  // If the user has navigated in the omnibox and NotifySessionTime is called,
  // then the educational promo should be seen.
  EXPECT_TRUE(instance->promo_seen());

  DesktopSessionDurationTracker::Get()->RemoveObserver(instance.release());
  manager()->DeleteTestingProfile("profile_3");
}

TEST_F(NewTabFeatureEngagementTrackerTest, TestOnSessionEnded) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_4");
  ASSERT_TRUE(profile);

  DesktopSessionDurationTracker::Initialize();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(profile);
  PrefService* prefs = profile->GetPrefs();

  instance->NotifyOmniboxNavigation();

  EXPECT_FALSE(prefs->GetBoolean(prefs::kNewTabInProductHelp));

  // Check that prefs are correctly configured.
  instance->OnSessionEnded(base::TimeDelta::FromHours(2));

  EXPECT_TRUE(prefs->GetBoolean(prefs::kNewTabInProductHelp));

  DesktopSessionDurationTracker::Get()->RemoveObserver(instance.release());
  manager()->DeleteTestingProfile("profile_4");
}

}  // namespace new_tab_feature_engagement_tracker
