// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/session_duration_updater.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/feature_engagement/feature_tracker.h"
#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "chrome/browser/feature_engagement/session_duration_updater_factory.h"
#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/feature_engagement/test/test_tracker.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {
// Name of default profile.
const char kTestProfileName[] = "test-profile";

class TestObserver : public SessionDurationUpdater::Observer {
 public:
  explicit TestObserver(SessionDurationUpdater* session_duration_updater)
      : session_duration_updater_(session_duration_updater),
        session_duration_observer_(this),
        pref_service_(
            base::MakeUnique<sync_preferences::TestingPrefServiceSyncable>()) {
    SessionDurationUpdater::RegisterProfilePrefs(pref_service_->registry());
  }

  void AddSessionDurationObserver() {
    session_duration_observer_.Add(session_duration_updater_);
  }

  void RemoveSessionDurationObserver() {
    session_duration_observer_.Add(session_duration_updater_);
  }

  void OnSessionEnded(base::TimeDelta total_session_time) override {}

  PrefService* GetPrefs() { return pref_service_.get(); }

  SessionDurationUpdater* GetSessionDurationUpdater() {
    return session_duration_updater_;
  }

 private:
  SessionDurationUpdater* const session_duration_updater_;
  ScopedObserver<SessionDurationUpdater, SessionDurationUpdater::Observer>
      session_duration_observer_;

  const std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      pref_service_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class SessionDurationUpdaterTest : public testing::Test {
 public:
  SessionDurationUpdaterTest() = default;
  ~SessionDurationUpdaterTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();
    testing_profile_manager_ = base::MakeUnique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(testing_profile_manager_->SetUp());

    test_observer_ = base::MakeUnique<TestObserver>(
        feature_engagement::SessionDurationUpdaterFactory::GetInstance()
            ->GetForProfile(testing_profile_manager_->CreateTestingProfile(
                kTestProfileName)));
  }
  void TearDown() override {
    metrics::DesktopSessionDurationTracker::CleanupForTesting();
    testing_profile_manager_->DeleteTestingProfile(kTestProfileName);
    testing_profile_manager_.reset();
    test_observer_->RemoveSessionDurationObserver();
  }

 protected:
  std::unique_ptr<TestingProfileManager> testing_profile_manager_;
  std::unique_ptr<TestObserver> test_observer_;

 private:
  content::TestBrowserThreadBundle thread_bundle_;

  DISALLOW_COPY_AND_ASSIGN(SessionDurationUpdaterTest);
};

}  // namespace

TEST_F(SessionDurationUpdaterTest, TestTimeAdded) {
  // Tests 50 minutes passing with an observer added.
  test_observer_->AddSessionDurationObserver();

  test_observer_->OnSessionEnded(base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(
      50, test_observer_->GetPrefs()->GetInteger(prefs::kObservedSessionTime));
}

TEST_F(SessionDurationUpdaterTest, TestAddingAndRemovingObservers) {
  // Tests 50 minutes passing with an observer added.
  test_observer_->AddSessionDurationObserver();

  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(
      50, test_observer_->GetPrefs()->GetInteger(prefs::kObservedSessionTime));

  // Tests 50 minutes passing without any observers. No time should be added to
  // the pref in this case.
  test_observer_->RemoveSessionDurationObserver();

  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(
      50, test_observer_->GetPrefs()->GetInteger(prefs::kObservedSessionTime));

  // Tests 50 minutes passing with an observer re-added. Time should be added
  // again now.
  test_observer_->AddSessionDurationObserver();

  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromMinutes(50));

  EXPECT_EQ(
      100, test_observer_->GetPrefs()->GetInteger(prefs::kObservedSessionTime));
}

}  // namespace feature_engagement
