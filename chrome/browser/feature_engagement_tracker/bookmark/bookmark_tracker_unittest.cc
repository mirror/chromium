// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/bookmark/bookmark_tracker.h"

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

namespace feature_engagement_tracker {

namespace {

const char kGroupName[] = "Enabled";
const char kBookmarkTrialName[] = "BookmarkTrial";

class MockFeatureEngagementTracker : public FeatureEngagementTracker {
 public:
  MockFeatureEngagementTracker() = default;
  MOCK_METHOD1(NotifyEvent, void(const std::string& event));
  MOCK_METHOD1(ShouldTriggerHelpUI, bool(const base::Feature& feature));
  MOCK_METHOD1(Dismissed, void(const base::Feature& feature));
  MOCK_METHOD0(IsInitialized, bool());
  MOCK_METHOD1(AddOnInitializedCallback, void(OnInitializedCallback callback));
};

class FakeBookmarkTracker : public BookmarkTracker {
 public:
  explicit FakeBookmarkTracker(FeatureEngagementTracker* feature_tracker)
      : feature_tracker_(feature_tracker),
        pref_service_(
            base::MakeUnique<sync_preferences::TestingPrefServiceSyncable>()) {
    BookmarkTracker::RegisterProfilePrefs(pref_service_->registry());
  }

  // feature_engagement_tracker::BookmarkTracker::
  FeatureEngagementTracker* GetFeatureTracker() override {
    return feature_tracker_;
  }

  PrefService* GetPrefs() override { return pref_service_.get(); }

 private:
  FeatureEngagementTracker* const feature_tracker_;
  const std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      pref_service_;
};

class BookmarkTrackerEventTest : public testing::Test {
 public:
  BookmarkTrackerEventTest() = default;
  ~BookmarkTrackerEventTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();
    mock_feature_tracker_ =
        base::MakeUnique<testing::StrictMock<MockFeatureEngagementTracker>>();
    bookmark_tracker_ =
        base::MakeUnique<FakeBookmarkTracker>(mock_feature_tracker_.get());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        bookmark_tracker_.get());
    metrics::DesktopSessionDurationTracker::CleanupForTesting();
  }

 protected:
  std::unique_ptr<FakeBookmarkTracker> bookmark_tracker_;
  std::unique_ptr<MockFeatureEngagementTracker> mock_feature_tracker_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkTrackerEventTest);
};

}  // namespace

// Tests to verify FeatureEngagementTracker API boundary expectations:

// If OnBookmarkAdded() is called, the FeatureEngagementTracker
// receives the kBookmarkAddedEvent.
TEST_F(BookmarkTrackerEventTest, TestOnBookmarkAdded) {
  EXPECT_CALL(*mock_feature_tracker_, NotifyEvent(events::kBookmarkAdded));
  bookmark_tracker_->OnBookmarkAdded();
}

// If OnSessionTimeMet() is called, the FeatureEngagementTracker
// receives the kSessionTime event.
TEST_F(BookmarkTrackerEventTest, TestOnSessionTimeMet) {
  EXPECT_CALL(*mock_feature_tracker_, NotifyEvent(events::kSessionTime));
  bookmark_tracker_->OnSessionTimeMet();
}

namespace {

class BookmarkTrackerFeatureEngagementTrackerTest : public testing::Test {
 public:
  BookmarkTrackerFeatureEngagementTrackerTest() = default;
  ~BookmarkTrackerFeatureEngagementTrackerTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Set up the BookmarkInProductHelp field trial.
    base::FieldTrial* bookmark_trial =
        base::FieldTrialList::CreateFieldTrial(kBookmarkTrialName, kGroupName);
    trials_[kIPHBookmarkFeature.name] = bookmark_trial;

    std::unique_ptr<base::FeatureList> feature_list =
        base::MakeUnique<base::FeatureList>();
    feature_list->RegisterFieldTrialOverride(
        kIPHBookmarkFeature.name, base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        bookmark_trial);

    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
    ASSERT_EQ(bookmark_trial,
              base::FeatureList::GetFieldTrial(kIPHBookmarkFeature));

    std::map<std::string, std::string> bookmark_params;
    bookmark_params["event_bookmark_added"] =
        "name:bookmark_added;comparator:==0;window:3650;storage:3650";
    bookmark_params["event_session_time"] =
        "name:session_time;comparator:>=1;window:3650;storage:3650";
    bookmark_params["event_trigger"] =
        "name:bookmark_trigger;comparator:any;window:3650;storage:3650";
    bookmark_params["event_used"] =
        "name:bookmark_clicked;comparator:any;window:3650;storage:3650";
    bookmark_params["session_rate"] = "<=3";
    bookmark_params["availability"] = "any";

    SetFeatureParams(kIPHBookmarkFeature, bookmark_params);

    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();

    feature_engagement_tracker_ = CreateTestFeatureEngagementTracker();

    bookmark_tracker_ = base::MakeUnique<FakeBookmarkTracker>(
        feature_engagement_tracker_.get());

    // The feature engagement tracker does async initialization.
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(feature_engagement_tracker_->IsInitialized());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        bookmark_tracker_.get());
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
  std::unique_ptr<FakeBookmarkTracker> bookmark_tracker_;
  std::unique_ptr<FeatureEngagementTracker> feature_engagement_tracker_;
  variations::testing::VariationParamsManager params_manager_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::map<std::string, base::FieldTrial*> trials_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkTrackerFeatureEngagementTrackerTest);
};

}  // namespace

// Tests to verify BookmarkFeatureEngagementTracker functional expectations:

// Test that a promo is not shown if the user has a Bookmark. If
// OnBookmarkAdded() is called, the ShouldShowPromo() should return false.
TEST_F(BookmarkTrackerFeatureEngagementTrackerTest, TestShouldNotShowPromo) {
  EXPECT_FALSE(bookmark_tracker_->ShouldShowPromo());

  bookmark_tracker_->OnSessionTimeMet();

  EXPECT_TRUE(bookmark_tracker_->ShouldShowPromo());

  bookmark_tracker_->OnBookmarkAdded();

  EXPECT_FALSE(bookmark_tracker_->ShouldShowPromo());
}

// Test that the prefs for session time is being correctly set.
TEST_F(BookmarkTrackerFeatureEngagementTrackerTest, TestPrefs) {
  EXPECT_FALSE(
      bookmark_tracker_->GetPrefs()->GetBoolean(prefs::kBookmarkInProductHelp));

  bookmark_tracker_->OnSessionTimeMet();
  bookmark_tracker_->OnVisitedKnownURL();

  EXPECT_TRUE(
      bookmark_tracker_->GetPrefs()->GetBoolean(prefs::kBookmarkInProductHelp));
}

// Test that the correct duration of session is being recorded.
TEST_F(BookmarkTrackerFeatureEngagementTrackerTest, TestOnSessionEnded) {
  metrics::DesktopSessionDurationTracker::Observer* observer =
      dynamic_cast<FakeBookmarkTracker*>(bookmark_tracker_.get());

  // Simulate passing 30 active minutes.
  observer->OnSessionEnded(base::TimeDelta::FromMinutes(30));

  EXPECT_EQ(
      30, bookmark_tracker_->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));

  // Simulate passing 50 minutes.
  observer->OnSessionEnded(base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(
      80, bookmark_tracker_->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));
}

}  // namespace feature_engagement_tracker
