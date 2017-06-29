// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/new_tab_feature_engagement_tracker.h"

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

namespace feature_engagement_tracker {

class MockNewTabFeatureEngagementTracker
    : public NewTabFeatureEngagementTracker {
 public:
  explicit MockNewTabFeatureEngagementTracker(Profile* profile)
      : testing_profile_(profile),
        notify_new_tab_opened_(false),
        notify_omnibox_navigation_(false),
        notify_session_time_(false),
        promo_seen_(false) {}

  bool promo_seen() const {
    return !notify_new_tab_opened_ && notify_omnibox_navigation_ &&
           notify_session_time_;
  }

  // feature_engagement_tracker::NewTabFeatureEngagementTracker
  Profile* GetProfile() override { return testing_profile_; }

  void NotifyNewTabOpened() override { notify_new_tab_opened_ = true; }

  void NotifyOmniboxNavigation() override { notify_omnibox_navigation_ = true; }

  void NotifySessionTime() override { notify_session_time_ = true; }

 private:
  bool promo_seen_;
  bool notify_new_tab_opened_;
  bool notify_omnibox_navigation_;
  bool notify_session_time_;
  Profile* testing_profile_;

  DISALLOW_COPY_AND_ASSIGN(MockNewTabFeatureEngagementTracker);
};

class MockFeatureEngagementTracker : public FeatureEngagementTracker {
 public:
  MockFeatureEngagementTracker() {}

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
  auto instance = base::MakeUnique<MockNewTabFeatureEngagementTracker>(profile);

  instance->NotifyNewTabOpened();

  // If a new tab is created, then the educational promo
  // should not be seen.
  EXPECT_FALSE(instance->promo_seen());

  delete instance;
}

TEST_F(NewTabFeatureEngagementTrackerTest, TestOmniboxNavigation) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_2");
  ASSERT_TRUE(profile);

  DesktopSessionDurationTracker::Initialize();
  auto instance = base::MakeUnique<MockNewTabFeatureEngagementTracker>(profile);

  EXPECT_FALSE(instance->promo_seen());

  // Two active hours have passed.
  instance->NotifySessionTime();

  instance->NotifyOmniboxNavigation();

  // If two hours have passed and the user has navigated in the omnibox,
  // then the educational promo should be seen.
  EXPECT_TRUE(instance->promo_seen());

  delete instance;
}

TEST_F(NewTabFeatureEngagementTrackerTest, TestNotifySessionTime) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_3");
  ASSERT_TRUE(profile);

  DesktopSessionDurationTracker::Initialize();
  auto instance = base::MakeUnique<MockNewTabFeatureEngagementTracker>(profile);

  EXPECT_FALSE(instance_->promo_seen());

  instance_->NotifyOmniboxNavigation();
  instance_->NotifySessionTime();

  // If the user has navigated in the omnibox and NotifySessionTime is called,
  // then the educational promo should be seen.
  EXPECT_TRUE(instance_->promo_seen());

  delete instance_;
}

}  // namespace feature_engagement_tracker
