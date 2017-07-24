// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/incognito_window/incognito_window_tracker.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/common/pref_names.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/feature_engagement_tracker/test/test_feature_engagement_tracker.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

MATCHER_P(IsFeature, feature, "") {
  return arg.name == feature.name;
}

namespace feature_engagement_tracker {

namespace {

const char kGroupName[] = "Enabled";
const char kIncognitoWindowTrialName[] = "IncognitoWindowTrial";

class MockFeatureEngagementTracker : public FeatureEngagementTracker {
 public:
  MockFeatureEngagementTracker() = default;
  MOCK_METHOD1(NotifyEvent, void(const std::string& event));
  MOCK_METHOD1(ShouldTriggerHelpUI, bool(const base::Feature& feature));
  MOCK_METHOD1(Dismissed, void(const base::Feature& feature));
  MOCK_METHOD0(IsInitialized, bool());
  MOCK_METHOD1(AddOnInitializedCallback, void(OnInitializedCallback callback));
};

class FakeIncognitoWindowTracker : public IncognitoWindowTracker {
 public:
  explicit FakeIncognitoWindowTracker(FeatureEngagementTracker* feature_tracker)
      : feature_tracker_(feature_tracker),
        pref_service_(
            base::MakeUnique<sync_preferences::TestingPrefServiceSyncable>()) {
    IncognitoWindowTracker::RegisterProfilePrefs(pref_service_->registry());
  }

  // feature_engagement_tracker::IncognitoWindowTracker::
  FeatureEngagementTracker* GetFeatureTracker() override {
    return feature_tracker_;
  }

  PrefService* GetPrefs() override { return pref_service_.get(); }

 private:
  FeatureEngagementTracker* const feature_tracker_;
  const std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      pref_service_;
};

class IncognitoWindowTrackerEventTest : public testing::Test {
 public:
  IncognitoWindowTrackerEventTest() = default;
  ~IncognitoWindowTrackerEventTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();
    mock_feature_tracker_ =
        base::MakeUnique<testing::StrictMock<MockFeatureEngagementTracker>>();
    incognito_window_tracker_ = base::MakeUnique<FakeIncognitoWindowTracker>(
        mock_feature_tracker_.get());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        incognito_window_tracker_.get());
    metrics::DesktopSessionDurationTracker::CleanupForTesting();
  }

 protected:
  std::unique_ptr<FakeIncognitoWindowTracker> incognito_window_tracker_;
  std::unique_ptr<MockFeatureEngagementTracker> mock_feature_tracker_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowTrackerEventTest);
};

}  // namespace

// Tests to verify FeatureEngagementTracker API boundary expectations:

// If OnIncognitoWindowOpened() is called, the FeatureEngagementTracker
// receives the kIncognitoWindowOpenedEvent.
TEST_F(IncognitoWindowTrackerEventTest, TestOnIncognitoWindowOpened) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(events::kIncognitoWindowOpened));
  incognito_window_tracker_->OnIncognitoWindowOpened();
}

// If OnOHistoryDeleted() is called, the FeatureEngagementTracker
// receives the kHistoryDeleted event.
TEST_F(IncognitoWindowTrackerEventTest, TestOnHistoryDeleted) {
  EXPECT_CALL(*mock_feature_tracker_, NotifyEvent(events::kHistoryDeleted));
  EXPECT_CALL(*mock_feature_tracker_,
              ShouldTriggerHelpUI(IsFeature(
                  feature_engagement_tracker::kIPHIncognitoWindowFeature)));
  incognito_window_tracker_->OnHistoryDeleted();
}

// If OnSessionTimeMet() is called, the FeatureEngagementTracker
// receives the kSessionTime event.
TEST_F(IncognitoWindowTrackerEventTest, TestOnSessionTimeMet) {
  EXPECT_CALL(*mock_feature_tracker_, NotifyEvent(events::kSessionTime));
  incognito_window_tracker_->OnSessionTimeMet();
}

namespace {

class IncognitoWindowFeatureEngagementTrackerTest : public testing::Test {
 public:
  IncognitoWindowFeatureEngagementTrackerTest() = default;
  ~IncognitoWindowFeatureEngagementTrackerTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Set up the IncognitoWindowInProductHelp field trial.
    base::FieldTrial* incognito_window_trial =
        base::FieldTrialList::CreateFieldTrial(kIncognitoWindowTrialName,
                                               kGroupName);
    trials_[kIPHIncognitoWindowFeature.name] = incognito_window_trial;

    std::unique_ptr<base::FeatureList> feature_list =
        base::MakeUnique<base::FeatureList>();
    feature_list->RegisterFieldTrialOverride(
        kIPHIncognitoWindowFeature.name,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, incognito_window_trial);

    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
    ASSERT_EQ(incognito_window_trial,
              base::FeatureList::GetFieldTrial(kIPHIncognitoWindowFeature));

    std::map<std::string, std::string> incognito_window_params;
    incognito_window_params["event_incognito_window_opened"] =
        "name:incognito_window_opened;comparator:==0;window:3650;storage:3650";
    incognito_window_params["event_history_deleted"] =
        "name:history_deleted;comparator:>=1;window:3650;storage:3650";
    incognito_window_params["event_session_time"] =
        "name:session_time;comparator:>=1;window:3650;storage:3650";
    incognito_window_params["event_trigger"] =
        "name:incognito_window_trigger;comparator:any;window:3650;storage:3650";
    incognito_window_params["event_used"] =
        "name:incognito_window_clicked;comparator:any;window:3650;storage:3650";
    incognito_window_params["session_rate"] = "<=3";
    incognito_window_params["availability"] = "any";

    SetFeatureParams(kIPHIncognitoWindowFeature, incognito_window_params);

    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();

    feature_engagement_tracker_ = CreateTestFeatureEngagementTracker();

    incognito_window_tracker_ = base::MakeUnique<FakeIncognitoWindowTracker>(
        feature_engagement_tracker_.get());

    // The feature engagement tracker does async initialization.
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(feature_engagement_tracker_->IsInitialized());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        incognito_window_tracker_.get());
    metrics::DesktopSessionDurationTracker::CleanupForTesting();

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
  std::unique_ptr<FakeIncognitoWindowTracker> incognito_window_tracker_;
  std::unique_ptr<FeatureEngagementTracker> feature_engagement_tracker_;
  variations::testing::VariationParamsManager params_manager_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::map<std::string, base::FieldTrial*> trials_;

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowFeatureEngagementTrackerTest);
};

}  // namespace

// Tests to verify IncognitoWindowFeatureEngagementTracker functional
// expectations:

// Test that a promo is not shown if the user uses an Incognito Window.
// If OnIncognitoWindowOpened() is called, the ShouldShowPromo() should return
// false.
TEST_F(IncognitoWindowFeatureEngagementTrackerTest, TestShouldNotShowPromo) {
  EXPECT_FALSE(incognito_window_tracker_->ShouldShowPromo());

  incognito_window_tracker_->OnSessionTimeMet();
  incognito_window_tracker_->OnHistoryDeleted();

  EXPECT_TRUE(incognito_window_tracker_->ShouldShowPromo());

  incognito_window_tracker_->OnIncognitoWindowOpened();

  EXPECT_FALSE(incognito_window_tracker_->ShouldShowPromo());
}

// Test that a promo is shown if the session time is met and a browsing history
// deletion occurs. If OnSessionTimeMet() and OnHistoryDeleted()
// are called, ShouldShowPromo() should return true.
TEST_F(IncognitoWindowFeatureEngagementTrackerTest, TestShouldShowPromo) {
  EXPECT_FALSE(incognito_window_tracker_->ShouldShowPromo());

  incognito_window_tracker_->OnSessionTimeMet();
  incognito_window_tracker_->OnHistoryDeleted();

  EXPECT_TRUE(incognito_window_tracker_->ShouldShowPromo());
}

// Test that the prefs for session time is being correctly set.
TEST_F(IncognitoWindowFeatureEngagementTrackerTest, TestPrefs) {
  EXPECT_FALSE(incognito_window_tracker_->GetPrefs()->GetBoolean(
      prefs::kIncognitoWindowInProductHelp));

  incognito_window_tracker_->OnSessionTimeMet();
  incognito_window_tracker_->OnHistoryDeleted();

  EXPECT_TRUE(incognito_window_tracker_->GetPrefs()->GetBoolean(
      prefs::kIncognitoWindowInProductHelp));
}

// Test that the correct duration of session is being recorded.
TEST_F(IncognitoWindowFeatureEngagementTrackerTest, TestOnSessionEnded) {
  metrics::DesktopSessionDurationTracker::Observer* observer =
      dynamic_cast<FakeIncognitoWindowTracker*>(
          incognito_window_tracker_.get());

  // Simulate passing 30 active minutes.
  observer->OnSessionEnded(base::TimeDelta::FromMinutes(30));

  EXPECT_EQ(30, incognito_window_tracker_->GetPrefs()->GetInteger(
                    prefs::kSessionTimeTotal));

  // Simulate passing 50 minutes.
  observer->OnSessionEnded(base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(80, incognito_window_tracker_->GetPrefs()->GetInteger(
                    prefs::kSessionTimeTotal));
}

}  // namespace feature_engagement_tracker
