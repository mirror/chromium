// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_tracker.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/feature_engagement_tracker/public/feature_list.h"
#include "components/feature_engagement_tracker/test/test_feature_engagement_tracker.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::StrictMock;

MATCHER_P(IsFeature, feature, "") {
  return arg.name == feature.name;
}

namespace new_tab_help {

const char kGroupName[] = "Enabled";
const char kNewTabTrialName[] = "NewTabTrial";

class MockFeatureEngagementTracker
    : public feature_engagement_tracker::FeatureEngagementTracker {
 public:
  MockFeatureEngagementTracker() = default;
  MOCK_METHOD1(NotifyEvent, void(const std::string& event));
  MOCK_METHOD1(ShouldTriggerHelpUI, bool(const base::Feature& feature));
  MOCK_METHOD1(Dismissed, void(const base::Feature& feature));
  MOCK_METHOD0(IsInitialized, bool());
  MOCK_METHOD1(AddOnInitializedCallback, void(OnInitializedCallback callback));
};

class FakeNewTabTracker : public new_tab_help::NewTabTracker {
 public:
  FakeNewTabTracker(
      feature_engagement_tracker::FeatureEngagementTracker* feature_tracker)
      : feature_tracker_(feature_tracker),
        pref_service_(base::MakeUnique<TestingPrefServiceSimple>()) {
    RegisterPrefs();
  }

  // new_tab_help::NewTabTracker::
  feature_engagement_tracker::FeatureEngagementTracker* GetFeatureTracker()
      override {
    return feature_tracker_;
  }

  PrefService* GetPrefs() override { return pref_service_.get(); }

  void RegisterPrefs() {
    PrefRegistrySimple* registry;
    registry = pref_service_->registry();
    registry->RegisterBooleanPref(prefs::kNewTabInProductHelp, false);
    registry->RegisterIntegerPref(prefs::kSessionTimeTotal, 0);
  }

 private:
  feature_engagement_tracker::FeatureEngagementTracker* const feature_tracker_;
  const std::unique_ptr<TestingPrefServiceSimple> pref_service_;
};

class NewTabTrackerEventTest : public testing::Test {
 public:
  NewTabTrackerEventTest() = default;
  ~NewTabTrackerEventTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();
    mock_feature_tracker_ =
        base::MakeUnique<StrictMock<MockFeatureEngagementTracker>>();
    new_tab_tracker_ =
        base::MakeUnique<FakeNewTabTracker>(mock_feature_tracker_.get());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        new_tab_tracker_.get());
  }

 protected:
  std::unique_ptr<FakeNewTabTracker> new_tab_tracker_;
  std::unique_ptr<MockFeatureEngagementTracker> mock_feature_tracker_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(NewTabTrackerEventTest);
};

// Tests to verify FeatureEngagementTracker API boundary expectations:

// If NotifyNewTabOpened() is called, the FeatureEngagementTracker
// receives the kNewTabOpenedEvent
TEST_F(NewTabTrackerEventTest, TestNotifyNewTabOpened) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(feature_engagement_tracker::events::kNewTabOpened));
  new_tab_tracker_->NotifyNewTabOpened();
}

// If NotifyOmniboxNavigation() is called, the FeatureEngagementTracker
// receives the kOmniboxInteraction event.
TEST_F(NewTabTrackerEventTest, TestNotifyOmniboxNavigation) {
  EXPECT_CALL(
      *mock_feature_tracker_,
      NotifyEvent(feature_engagement_tracker::events::kOmniboxInteraction));
  new_tab_tracker_->NotifyOmniboxNavigation();
}

// If NotifySessionTimeMet() is called, the FeatureEngagementTracker
// receives the kSessionTime event.
TEST_F(NewTabTrackerEventTest, TestNotifySessionTimeMet) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(feature_engagement_tracker::events::kSessionTime));
  new_tab_tracker_->NotifySessionTimeMet();
}

class NewTabTrackerFeatureEngagementTrackerTest : public testing::Test {
 public:
  NewTabTrackerFeatureEngagementTrackerTest() = default;
  ~NewTabTrackerFeatureEngagementTrackerTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Set up the NewTabInProductHelp field trial.
    base::FieldTrial* new_tab_trial =
        base::FieldTrialList::CreateFieldTrial(kNewTabTrialName, kGroupName);
    trials_[feature_engagement_tracker::kIPHNewTabFeature.name] = new_tab_trial;

    std::unique_ptr<base::FeatureList> feature_list =
        base::MakeUnique<base::FeatureList>();
    feature_list->RegisterFieldTrialOverride(
        feature_engagement_tracker::kIPHNewTabFeature.name,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, new_tab_trial);

    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
    ASSERT_EQ(new_tab_trial,
              base::FeatureList::GetFieldTrial(
                  feature_engagement_tracker::kIPHNewTabFeature));

    std::map<std::string, std::string> new_tab_params;
    new_tab_params["event_new_tab_opened"] =
        "name:new_tab_opened;comparator:==0;window:3650;storage:3650";
    new_tab_params["event_omnibox_used"] =
        "name:omnibox_used;comparator:>=1;window:3650;storage:3650";
    new_tab_params["event_session_time"] =
        "name:session_time;comparator:>=1;window:3650;storage:3650";
    new_tab_params["event_trigger"] =
        "name:new_tab_trigger;comparator:any;window:3650;storage:3650";
    new_tab_params["event_used"] =
        "name:new_tab_clicked;comparator:any;window:3650;storage:3650";
    new_tab_params["session_rate"] = "<=3";
    new_tab_params["availability"] = "any";

    SetFeatureParams(feature_engagement_tracker::kIPHNewTabFeature,
                     new_tab_params);

    metrics::DesktopSessionDurationTracker::Initialize();

    feature_engagement_tracker_ =
        feature_engagement_tracker::TestFeatureEngagementTracker::Create();

    new_tab_tracker_ =
        base::MakeUnique<FakeNewTabTracker>(feature_engagement_tracker_.get());

    // The feature engagement tracker does async initialization.
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(feature_engagement_tracker_->IsInitialized());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        new_tab_tracker_.get());

    // This is required to ensure each test can define its own params.
    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
  }

  void SetFeatureParams(const base::Feature& feature,
                        std::map<std::string, std::string> params) {
    ASSERT_TRUE(
        base::FieldTrialParamAssociator::GetInstance()
            ->AssociateFieldTrialParams(trials_[feature.name]->trial_name(),
                                        kGroupName, params));

    std::map<std::string, std::string> actualParams;
    EXPECT_TRUE(base::GetFieldTrialParamsByFeature(feature, &actualParams));
    EXPECT_EQ(params, actualParams);
  }

 protected:
  std::unique_ptr<FakeNewTabTracker> new_tab_tracker_;
  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      feature_engagement_tracker_;
  variations::testing::VariationParamsManager params_manager_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::map<std::string, base::FieldTrial*> trials_;

  DISALLOW_COPY_AND_ASSIGN(NewTabTrackerFeatureEngagementTrackerTest);
};

// Tests to verify NewTabFeatureEngagementTracker functional expectations:

// Test that a promo is not shown if the user uses a New Tab.
// If NotifyNewTabOpened() is called, the ShouldShowPromo() should return false.
TEST_F(NewTabTrackerFeatureEngagementTrackerTest, TestShouldNotShowPromo) {
  EXPECT_FALSE(new_tab_tracker_->ShouldShowPromo());

  new_tab_tracker_->NotifySessionTimeMet();
  new_tab_tracker_->NotifyOmniboxNavigation();

  EXPECT_TRUE(new_tab_tracker_->ShouldShowPromo());

  new_tab_tracker_->NotifyNewTabOpened();

  EXPECT_FALSE(new_tab_tracker_->ShouldShowPromo());
}

// Test that a promo is shown if the session time is met and an omnibox
// navigation occurs. If NotifySessionTimeMet() and NotifyOmniboxNavigation()
// are called, ShouldShowPromo() should return true.
TEST_F(NewTabTrackerFeatureEngagementTrackerTest, TestShouldShowPromo) {
  EXPECT_FALSE(new_tab_tracker_->ShouldShowPromo());

  new_tab_tracker_->NotifySessionTimeMet();
  new_tab_tracker_->NotifyOmniboxNavigation();

  EXPECT_TRUE(new_tab_tracker_->ShouldShowPromo());
}

// Test that the prefs for session time is being correctly set.
TEST_F(NewTabTrackerFeatureEngagementTrackerTest, TestPrefs) {
  EXPECT_FALSE(
      new_tab_tracker_->GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp));

  new_tab_tracker_->NotifySessionTimeMet();
  new_tab_tracker_->NotifyOmniboxNavigation();
  new_tab_tracker_->OmniboxOnFocus();

  EXPECT_TRUE(
      new_tab_tracker_->GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp));
}

// Test that the correct duration of session is being recorded.
TEST_F(NewTabTrackerFeatureEngagementTrackerTest, TestOnSessionEnded) {
  metrics::DesktopSessionDurationTracker::Observer* observer =
      dynamic_cast<FakeNewTabTracker*>(new_tab_tracker_.get());

  // Simulate passing 30 active minutes.
  observer->OnSessionEnded(base::TimeDelta::FromMinutes(30));

  EXPECT_EQ(30,
            new_tab_tracker_->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));

  // Simulate passing 50 minutes.
  observer->OnSessionEnded(base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(80,
            new_tab_tracker_->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));
}

}  // namespace new_tab_help
