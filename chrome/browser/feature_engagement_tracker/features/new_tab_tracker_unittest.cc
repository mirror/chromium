// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement_tracker/features/new_tab_tracker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_feature_list.h"
#include "base/version.h"
#include "chrome/browser/feature_engagement_tracker/feature_engagement_tracker_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/feature_engagement_tracker/internal/availability_model_impl.h"
#include "components/feature_engagement_tracker/internal/chrome_variations_configuration.h"
#include "components/feature_engagement_tracker/internal/condition_validator.h"
#include "components/feature_engagement_tracker/internal/editable_configuration.h"
#include "components/feature_engagement_tracker/internal/feature_config_condition_validator.h"
#include "components/feature_engagement_tracker/internal/feature_config_storage_validator.h"
#include "components/feature_engagement_tracker/internal/feature_engagement_tracker_impl.h"
#include "components/feature_engagement_tracker/internal/in_memory_store.h"
#include "components/feature_engagement_tracker/internal/init_aware_model.h"
#include "components/feature_engagement_tracker/internal/model_impl.h"
#include "components/feature_engagement_tracker/internal/never_availability_model.h"
#include "components/feature_engagement_tracker/internal/never_storage_validator.h"
#include "components/feature_engagement_tracker/internal/persistent_store.h"
#include "components/feature_engagement_tracker/internal/proto/availability.pb.h"
#include "components/feature_engagement_tracker/internal/stats.h"
#include "components/feature_engagement_tracker/internal/system_time_provider.h"
#include "components/feature_engagement_tracker/public/event_constants.h"
#include "components/feature_engagement_tracker/public/feature_constants.h"
#include "components/feature_engagement_tracker/public/feature_engagement_tracker.h"
#include "components/feature_engagement_tracker/public/feature_list.h"
#include "components/leveldb_proto/proto_database_impl.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::StrictMock;

namespace testing {
namespace internal {
bool operator==(const base::Feature& first, const base::Feature& second) {
  return first.name == second.name;
}
}  // namespace internal
}  // namespace testing

namespace new_tab_help {

const char kNewTabTrialName[] = "NewTabTrial";
const char kGroupName[] = "Enabled";

const base::FilePath::CharType kEventDBStorageDir[] =
    FILE_PATH_LITERAL("EventDB");
const base::FilePath::CharType kAvailabilityDBStorageDir[] =
    FILE_PATH_LITERAL("AvailabilityDB");

static const char profile_name[] = "profile";

class MockFeatureEngagementTracker
    : public feature_engagement_tracker::FeatureEngagementTracker {
 public:
  MockFeatureEngagementTracker() = default;
  MOCK_METHOD1(NotifyEvent, void(const std::string& event));
  MOCK_METHOD1(ShouldTriggerHelpUI, bool(const base::Feature& feature));
  MOCK_METHOD1(Dismissed, void(const base::Feature& feature));
  MOCK_METHOD0(IsInitialized, bool());
  MOCK_METHOD1(AddOnInitializedCallback, void(OnInitializedCallback callback));

  // Creates a FeatureEngagementTracker usable for these unit tests.
  static std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
  CreateFeatureEngagementTracker(Profile* profile) {
    std::vector<const base::Feature*> features = {
        &feature_engagement_tracker::kIPHNewTabFeature};

    scoped_refptr<base::SequencedTaskRunner> background_task_runner =
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::BACKGROUND});

    base::FilePath storage_dir = profile->GetPath().Append(
        chrome::kFeatureEngagementTrackerStorageDirname);

    std::unique_ptr<
        leveldb_proto::ProtoDatabase<feature_engagement_tracker::Event>>
        db = base::MakeUnique<leveldb_proto::ProtoDatabaseImpl<
            feature_engagement_tracker::Event>>(background_task_runner);

    base::FilePath event_storage_dir = storage_dir.Append(kEventDBStorageDir);
    auto store = base::MakeUnique<feature_engagement_tracker::PersistentStore>(
        event_storage_dir, std::move(db));

    auto configuration = base::MakeUnique<
        feature_engagement_tracker::ChromeVariationsConfiguration>();
    configuration->ParseFeatureConfigs(features);

    auto storage_validator = base::MakeUnique<
        feature_engagement_tracker::FeatureConfigStorageValidator>();
    storage_validator->InitializeFeatures(features, *configuration);

    auto raw_model = base::MakeUnique<feature_engagement_tracker::ModelImpl>(
        std::move(store), std::move(storage_validator));

    auto model = base::MakeUnique<feature_engagement_tracker::InitAwareModel>(
        std::move(raw_model));

    auto condition_validator = base::MakeUnique<
        feature_engagement_tracker::FeatureConfigConditionValidator>();

    return base::MakeUnique<
        feature_engagement_tracker::FeatureEngagementTrackerImpl>(
        std::move(model),
        base::MakeUnique<feature_engagement_tracker::NeverAvailabilityModel>(),
        std::move(configuration), std::move(condition_validator),
        base::MakeUnique<feature_engagement_tracker::SystemTimeProvider>());
  }
};

class FakeNewTabTracker : public new_tab_help::NewTabTracker {
 public:
  FakeNewTabTracker(
      Profile* profile,
      feature_engagement_tracker::FeatureEngagementTracker* feature_tracker)
      : profile_(profile),
        feature_tracker_(feature_tracker),
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
  Profile* const profile_;
  const std::unique_ptr<TestingPrefServiceSimple> pref_service_;
};

class NewTabTrackerEventTest : public testing::Test {
 public:
  NewTabTrackerEventTest() = default;
  ~NewTabTrackerEventTest() override = default;

  // testing::Test:
  void SetUp() override {
    profile_manager_ = base::MakeUnique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(profile_name);
    ASSERT_TRUE(profile_);

    metrics::DesktopSessionDurationTracker::Initialize();
    mock_feature_tracker_ =
        base::MakeUnique<StrictMock<MockFeatureEngagementTracker>>();
    new_tab_tracker_ = base::MakeUnique<FakeNewTabTracker>(
        profile_, mock_feature_tracker_.get());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        new_tab_tracker_.get());
    profile_manager_->DeleteTestingProfile(profile_name);
  }

 protected:
  std::unique_ptr<FakeNewTabTracker> new_tab_tracker_;
  std::unique_ptr<MockFeatureEngagementTracker> mock_feature_tracker_;
  Profile* profile_;

 private:
  std::unique_ptr<TestingProfileManager> profile_manager_;
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(NewTabTrackerEventTest);
};

// Tests to verify FeatureEngagementTracker API boundary expectations:

// If NotifySessionTimeMet() is called, the FeatureEngagementTracker
// receives the kSessionTime event.
TEST_F(NewTabTrackerEventTest, TestNotifySessionTimeMet) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(feature_engagement_tracker::events::kSessionTime));
  // NotifySessionTimeMet does not expect a ShouldTriggerHelpUI() call since
  // the promo cannot be shown if the promo has ended.
  new_tab_tracker_->NotifySessionTimeMet();
}

// If NotifyOmniboxNavigation() is called, the FeatureEngagementTracker
// receives the kOmniboxInteraction event.
TEST_F(NewTabTrackerEventTest, TestNotifyOmniboxNavigation) {
  EXPECT_CALL(
      *mock_feature_tracker_,
      NotifyEvent(feature_engagement_tracker::events::kOmniboxInteraction));
  EXPECT_CALL(
      *mock_feature_tracker_,
      ShouldTriggerHelpUI(feature_engagement_tracker::kIPHNewTabFeature));
  new_tab_tracker_->NotifyOmniboxNavigation();
}

// If NotifyNewTabOpened() is called, the FeatureEngagementTracker
// receives the kNewTabOpenedEvent
TEST_F(NewTabTrackerEventTest, TestNotifyNewTabOpened) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(feature_engagement_tracker::events::kNewTabOpened));
  EXPECT_CALL(
      *mock_feature_tracker_,
      ShouldTriggerHelpUI(feature_engagement_tracker::kIPHNewTabFeature));
  new_tab_tracker_->NotifyNewTabOpened();
}

class NewTabTrackerFeatureEngagementTrackerTest : public testing::Test {
 public:
  NewTabTrackerFeatureEngagementTrackerTest() = default;
  ~NewTabTrackerFeatureEngagementTrackerTest() override = default;

  // testing::Test:
  void SetUp() override {
    profile_manager_ = base::MakeUnique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(profile_name);
    ASSERT_TRUE(profile_);

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

    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();

    feature_engagement_tracker_ =
        MockFeatureEngagementTracker::CreateFeatureEngagementTracker(profile_);

    new_tab_tracker_ = base::MakeUnique<FakeNewTabTracker>(
        profile_, feature_engagement_tracker_.get());

    // The feature engagement tracker does async initialization.
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(feature_engagement_tracker_->IsInitialized());
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        new_tab_tracker_.get());
    profile_manager_->DeleteTestingProfile(profile_name);
    feature_engagement_tracker_.reset();
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
  Profile* profile_;

 private:
  std::unique_ptr<TestingProfileManager> profile_manager_;
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

// Test that the feature tracker works with an incognito profile.
TEST_F(NewTabTrackerFeatureEngagementTrackerTest, TestIncognitoProfile) {
  Profile* incognito_profile = profile_->GetOffTheRecordProfile();
  ASSERT_TRUE(incognito_profile);

  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      feature_engagement_tracker =
          MockFeatureEngagementTracker::CreateFeatureEngagementTracker(
              incognito_profile);
  // The feature engagement tracker does async initialization.
  base::RunLoop().RunUntilIdle();

  if (feature_engagement_tracker->IsInitialized()) {
    auto new_tab_tracker = base::MakeUnique<FakeNewTabTracker>(
        profile_, feature_engagement_tracker.get());

    EXPECT_FALSE(new_tab_tracker->ShouldShowPromo());

    new_tab_tracker->NotifySessionTimeMet();
    new_tab_tracker->NotifyOmniboxNavigation();

    EXPECT_TRUE(new_tab_tracker->ShouldShowPromo());
  }
}

}  // namespace new_tab_help
