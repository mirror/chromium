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

using testing::Mock;

namespace {

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

class FakeNewTabTracker : public new_tab_iph::NewTabTracker {
 public:
  FakeNewTabTracker(Profile* profile,
                    MockFeatureEngagementTracker* feature_tracker)
      : profile_(profile),
        feature_tracker_(feature_tracker),
        pref_service_(base::MakeUnique<TestingPrefServiceSimple>()) {
    RegisterPrefs();
  }

  // new_tab_iph::NewTabTracker::
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
  MockFeatureEngagementTracker* const feature_tracker_;
  Profile* const profile_;
  const std::unique_ptr<TestingPrefServiceSimple> pref_service_;
};

class MockNewTabTracker : public new_tab_iph::NewTabTracker {
 public:
  MockNewTabTracker(
      Profile* profile,
      feature_engagement_tracker::FeatureEngagementTracker* feature_tracker)
      : profile_(profile),
        feature_tracker_(feature_tracker),
        pref_service_(base::MakeUnique<TestingPrefServiceSimple>()) {
    RegisterPrefs();
  }

  // new_tab_iph::NewTabTracker::
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

}  // namespace

namespace new_tab_iph {

const char kNewTabTrialName[] = "NewTabTrial";
const char kGroupName[] = "Enabled";

const base::FilePath::CharType kEventDBStorageDir[] =
    FILE_PATH_LITERAL("EventDB");
const base::FilePath::CharType kAvailabilityDBStorageDir[] =
    FILE_PATH_LITERAL("AvailabilityDB");

class NewTabTrackerTest : public testing::Test {
 public:
  NewTabTrackerTest() {
    base::FieldTrial* new_tab_trial =
        base::FieldTrialList::CreateFieldTrial(kNewTabTrialName, kGroupName);
    trials_[feature_engagement_tracker::kIPHNewTabFeature.name] = new_tab_trial;

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        feature_engagement_tracker::kIPHNewTabFeature.name,
        base::FeatureList::OVERRIDE_ENABLE_FEATURE, new_tab_trial);

    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
    EXPECT_EQ(new_tab_trial,
              base::FeatureList::GetFieldTrial(
                  feature_engagement_tracker::kIPHNewTabFeature));
  }
  ~NewTabTrackerTest() override {}

  // testing::Test:
  void SetUp() override {
    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("profile");
    ASSERT_TRUE(profile_);

    metrics::DesktopSessionDurationTracker::Initialize();
    mock_feature_tracker_ = new MockFeatureEngagementTracker();
    instance_ =
        base::MakeUnique<FakeNewTabTracker>(profile_, mock_feature_tracker_);

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
  }

  void TearDown() override {
    metrics::DesktopSessionDurationTracker::Get()->RemoveObserver(
        instance_.release());
    profile_manager_->DeleteTestingProfile("profile");
    delete mock_feature_tracker_;

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

  // Creates a FeatureEngagementTracker usable for these unit tests.
  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
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

 protected:
  std::unique_ptr<FakeNewTabTracker> instance_;
  MockFeatureEngagementTracker* mock_feature_tracker_;
  variations::testing::VariationParamsManager params_manager_;
  Profile* profile_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::map<std::string, base::FieldTrial*> trials_;

  DISALLOW_COPY_AND_ASSIGN(NewTabTrackerTest);
};

// Tests to verify FeatureEngagementTracker API boundary expectations:

// If NotifySessionTimeMet() is called, the FeatureEngagementTracker
// receives the kSessionTime event.
TEST_F(NewTabTrackerTest, TestNotifySessionTimeMet) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(feature_engagement_tracker::events::kSessionTime));
  instance_->NotifySessionTimeMet();
}

// If NotifyOmniboxNavigation() is called, the FeatureEngagementTracker
// receives the kOmniboxInteraction event.
TEST_F(NewTabTrackerTest, TestNotifyOmniboxNavigation) {
  EXPECT_CALL(
      *mock_feature_tracker_,
      NotifyEvent(feature_engagement_tracker::events::kOmniboxInteraction));
  instance_->NotifyOmniboxNavigation();
}

// If NotifyNewTabOpened() is called, the FeatureEngagementTracker
// receives the kNewTabOpenedEvent
TEST_F(NewTabTrackerTest, TestNotifyNewTabOpened) {
  EXPECT_CALL(*mock_feature_tracker_,
              NotifyEvent(feature_engagement_tracker::events::kNewTabOpened));
  instance_->NotifyNewTabOpened();
}

// Tests to verify NewTabFeatureEngagementTracker functional expectations:

// Test that a promo is not shown if the user uses a New Tab.
// If NotifyNewTabOpened() is called, the ShouldShowPromo() should return false.
TEST_F(NewTabTrackerTest, TestShouldNotShowPromo) {
  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      real_feature_tracker = CreateFeatureEngagementTracker(profile_);
  base::RunLoop().RunUntilIdle();

  if (real_feature_tracker->IsInitialized()) {
    auto instance = base::MakeUnique<MockNewTabTracker>(
        profile_, real_feature_tracker.get());

    EXPECT_FALSE(instance->ShouldShowPromo());

    instance->NotifySessionTimeMet();
    instance->NotifyOmniboxNavigation();

    EXPECT_TRUE(instance->ShouldShowPromo());

    instance->NotifyNewTabOpened();

    EXPECT_FALSE(instance->ShouldShowPromo());
  }
}

// Test that a promo is shown if the session time is met and an omnibox
// navigation occurs. If NotifySessionTimeMet() and NotifyOmniboxNavigation()
// are called, ShouldShowPromo() should return true.
TEST_F(NewTabTrackerTest, TestShouldShowPromo) {
  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      real_feature_tracker = CreateFeatureEngagementTracker(profile_);
  base::RunLoop().RunUntilIdle();

  if (real_feature_tracker->IsInitialized()) {
    auto instance = base::MakeUnique<MockNewTabTracker>(
        profile_, real_feature_tracker.get());

    EXPECT_FALSE(instance->ShouldShowPromo());

    instance->NotifySessionTimeMet();
    instance->NotifyOmniboxNavigation();

    EXPECT_TRUE(instance->ShouldShowPromo());
  }
}

// Test that the prefs for session time is being correctly set.
TEST_F(NewTabTrackerTest, TestPrefs) {
  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      real_feature_tracker = CreateFeatureEngagementTracker(profile_);
  base::RunLoop().RunUntilIdle();

  if (real_feature_tracker->IsInitialized()) {
    auto instance = base::MakeUnique<MockNewTabTracker>(
        profile_, real_feature_tracker.get());

    EXPECT_FALSE(instance->GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp));

    instance->NotifySessionTimeMet();
    instance->NotifyOmniboxNavigation();

    // Check that prefs are correctly configured.
    EXPECT_TRUE(instance->GetPrefs()->GetBoolean(prefs::kNewTabInProductHelp));
  }
}

// Test that the correct duration of session is being recorded.
TEST_F(NewTabTrackerTest, TestOnSessionEnded) {
  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      real_feature_tracker = CreateFeatureEngagementTracker(profile_);
  base::RunLoop().RunUntilIdle();

  if (real_feature_tracker->IsInitialized()) {
    auto instance = base::MakeUnique<MockNewTabTracker>(
        profile_, real_feature_tracker.get());

    metrics::DesktopSessionDurationTracker::Observer* observer =
        dynamic_cast<MockNewTabTracker*>(instance.get());

    // 30 active minutes passed.
    observer->OnSessionEnded(base::TimeDelta::FromMinutes(30));

    EXPECT_EQ(30, instance->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));

    // 50 active minutes passed.
    observer->OnSessionEnded(base::TimeDelta::FromMinutes(50));

    // Elapsed time should be 80 minutes.
    EXPECT_EQ(80, instance->GetPrefs()->GetInteger(prefs::kSessionTimeTotal));
  }
}

// Test that the feature tracker works with an incognito profile.
TEST_F(NewTabTrackerTest, TestIncognitoProfile) {
  Profile* incognito_profile = profile_->GetOffTheRecordProfile();
  ASSERT_TRUE(incognito_profile);

  std::unique_ptr<feature_engagement_tracker::FeatureEngagementTracker>
      real_feature_tracker = CreateFeatureEngagementTracker(incognito_profile);
  base::RunLoop().RunUntilIdle();

  if (real_feature_tracker->IsInitialized()) {
    auto instance = base::MakeUnique<MockNewTabTracker>(
        profile_, real_feature_tracker.get());

    EXPECT_FALSE(instance->ShouldShowPromo());

    instance->NotifySessionTimeMet();
    instance->NotifyOmniboxNavigation();

    EXPECT_TRUE(instance->ShouldShowPromo());
  }
}

}  // namespace new_tab_iph
