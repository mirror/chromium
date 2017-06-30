// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_feature_engagement_tracker.h"

#include <string>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement_tracker {

class FakeFeatureEngagementTracker
    : public feature_engagement_tracker::FeatureEngagementTracker {
 public:
  FakeFeatureEngagementTracker()
      : notify_new_tab_opened_(false),
        notify_omnibox_navigation_(false),
        notify_session_time_met_(false) {}

  // feature_engagement_tracker::FeatureEngagementTracker
  void NotifyEvent(const std::string& event) {
    if (!event.compare(feature_engagement_tracker::events::kNewTabOpened)) {
      notify_new_tab_opened_ = true;
    } else if (!event.compare(
                   feature_engagement_tracker::events::kOmniboxInteraction)) {
      notify_omnibox_navigation_ = true;
    } else if (!event.compare(
                   feature_engagement_tracker::events::kSessionTime)) {
      notify_session_time_met_ = true;
    }
  }

  bool ShouldTriggerHelpUI(const base::Feature& feature) override {
    DVLOG(4) << notify_new_tab_opened_ << notify_omnibox_navigation_
             << notify_session_time_met_;
    return !notify_new_tab_opened_ && notify_omnibox_navigation_ &&
           notify_session_time_met_;
  }

  void Dismissed(const base::Feature& feature) override {}
  bool IsInitialized() override { return true; }
  void AddOnInitializedCallback(OnInitializedCallback callback) override {}

 private:
  bool notify_new_tab_opened_;
  bool notify_omnibox_navigation_;
  bool notify_session_time_met_;
};

}  // namespace feature_engagement_tracker

namespace new_tab_feature_engagement_tracker {

class FakeNewTabFeatureEngagementTracker
    : public NewTabFeatureEngagementTracker {
 public:
  FakeNewTabFeatureEngagementTracker(
      Profile* profile,
      feature_engagement_tracker::FakeFeatureEngagementTracker* feature_tracker)
      : profile_(profile), feature_tracker_(feature_tracker) {
    RegisterPrefs();
  }

  // new_tab_feature_engagement_tracker::NewTabFeatureEngagementTracker
  feature_engagement_tracker::FeatureEngagementTracker* GetFeatureTracker()
      override {
    return feature_tracker_;
  }

  PrefService* GetPrefs() override { return pref_service_.get(); }

  void RegisterPrefs() {
    pref_service_.reset(new TestingPrefServiceSimple);
    registry_ = pref_service_->registry();
    registry_->RegisterBooleanPref(prefs::kNewTabInProductHelp, false);
    registry_->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
  }

 private:
  feature_engagement_tracker::FakeFeatureEngagementTracker* feature_tracker_;
  PrefRegistrySimple* registry_;
  Profile* profile_;
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
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

// Test that a promo is not shown if the user uses a New Tab.
TEST_F(NewTabFeatureEngagementTrackerTest, TestNotifyNewTabOpened) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_1");
  ASSERT_TRUE(profile);

  metrics::DesktopSessionDurationTracker::Initialize();
  auto feature_tracker = base::MakeUnique<
      feature_engagement_tracker::FakeFeatureEngagementTracker>();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(
      profile, feature_tracker.release());
  instance->RegisterPrefs();

  EXPECT_FALSE(instance->ShouldShowPromo());

  instance->NotifySessionTimeMet();
  instance->NotifyOmniboxNavigation();

  EXPECT_TRUE(instance->ShouldShowPromo());

  // If a new tab is created, then the educational promo
  // should not be seen even though the other requirements are fulfilled.
  instance->NotifyNewTabOpened();
  EXPECT_FALSE(instance->ShouldShowPromo());

  metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
      instance.release());
  manager()->DeleteTestingProfile("profile_1");
}

// Test that a promo can't be shown if only an omnibox navigation occurs.
// omnibox navigation occurs.
// A promo is shown if the session time is met and an
// omnibox navigation occurs.
TEST_F(NewTabFeatureEngagementTrackerTest, TestOmniboxNavigation) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_2");
  ASSERT_TRUE(profile);

  metrics::DesktopSessionDurationTracker::Initialize();
  auto feature_tracker = base::MakeUnique<
      feature_engagement_tracker::FakeFeatureEngagementTracker>();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(
      profile, feature_tracker.release());
  instance->RegisterPrefs();

  // No requirements have occured yet.
  EXPECT_FALSE(instance->ShouldShowPromo());

  // Two active hours have passed.
  instance->NotifySessionTimeMet();

  // Not all requirements have been met.
  EXPECT_FALSE(instance->ShouldShowPromo());

  instance->NotifyOmniboxNavigation();

  // If two hours have passed and the user has navigated in the omnibox,
  // then the educational promo should be seen.
  EXPECT_TRUE(instance->ShouldShowPromo());

  metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
      instance.release());
  manager()->DeleteTestingProfile("profile_2");
}

// Test that a promo can't be shown if only the session time is met.
// A promo is shown if the session time is met and an
// omnibox navigation occurs.
TEST_F(NewTabFeatureEngagementTrackerTest, TestNotifySessionTimeMet) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_3");
  ASSERT_TRUE(profile);

  metrics::DesktopSessionDurationTracker::Initialize();
  auto feature_tracker = base::MakeUnique<
      feature_engagement_tracker::FakeFeatureEngagementTracker>();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(
      profile, feature_tracker.release());
  instance->RegisterPrefs();

  // No requirements have occured yet.
  EXPECT_FALSE(instance->ShouldShowPromo());

  instance->NotifyOmniboxNavigation();

  // Not all requirements have been met.
  EXPECT_FALSE(instance->ShouldShowPromo());

  instance->NotifySessionTimeMet();

  // If the user has navigated in the omnibox and NotifySessionTime is called,
  // then the educational promo should be seen.
  EXPECT_TRUE(instance->ShouldShowPromo());

  metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
      instance.release());
  manager()->DeleteTestingProfile("profile_3");
}

// Test that the prefs for session time is being correctly set.
TEST_F(NewTabFeatureEngagementTrackerTest, TestPrefs) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_4");
  ASSERT_TRUE(profile);

  metrics::DesktopSessionDurationTracker::Initialize();
  auto feature_tracker = base::MakeUnique<
      feature_engagement_tracker::FakeFeatureEngagementTracker>();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(
      profile, feature_tracker.release());
  instance->RegisterPrefs();

  EXPECT_FALSE(instance->GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp));

  instance->NotifySessionTimeMet();
  instance->NotifyOmniboxNavigation();

  // Check that prefs are correctly configured.
  EXPECT_TRUE(instance->GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp));

  metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
      instance.release());
  manager()->DeleteTestingProfile("profile_4");
}

// Test that the correct duration of session is being recorded.
TEST_F(NewTabFeatureEngagementTrackerTest, TestOnSessionEnded) {
  TestingProfile* profile = manager()->CreateTestingProfile("profile_5");
  ASSERT_TRUE(profile);

  metrics::DesktopSessionDurationTracker::Initialize();
  auto feature_tracker = base::MakeUnique<
      feature_engagement_tracker::FakeFeatureEngagementTracker>();
  auto instance = base::MakeUnique<FakeNewTabFeatureEngagementTracker>(
      profile, feature_tracker.release());
  instance->RegisterPrefs();

  // 30 active minutes passed.
  instance->OnSessionEnded(base::TimeDelta::FromMinutes(30));

  EXPECT_EQ(30, instance->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));

  // 50 active minutes passed.
  instance->OnSessionEnded(base::TimeDelta::FromMinutes(50));

  // Elapsed time should be 80 minutes.
  EXPECT_EQ(80, instance->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));

  metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
      instance.release());
  manager()->DeleteTestingProfile("profile_5");
}

}  // namespace new_tab_feature_engagement_tracker
