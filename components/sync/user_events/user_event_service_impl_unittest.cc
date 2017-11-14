// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/user_event_service_impl.h"

#include "base/message_loop/message_loop.h"
#include "base/test/scoped_feature_list.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/model/model_type_store_test_util.h"
#include "components/sync/model/recording_model_type_change_processor.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/user_events/user_event_sync_bridge.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::FieldTrialList;
using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

const char kTrial1[] = "TrialNameOne";
const char kTrial2[] = "TrialNameTwo";
const char kTrial3[] = "TrialNameThree";
const char kGroup[] = "GroupName";

std::unique_ptr<UserEventSpecifics> Event() {
  return std::make_unique<UserEventSpecifics>();
}

std::unique_ptr<UserEventSpecifics> AsTest(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_test_event();
  return specifics;
}

std::unique_ptr<UserEventSpecifics> AsDetection(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_language_detection_event();
  return specifics;
}

std::unique_ptr<UserEventSpecifics> AsTrial(
    std::unique_ptr<UserEventSpecifics> specifics) {
  specifics->mutable_field_trial_event();
  return specifics;
}

std::unique_ptr<UserEventSpecifics> WithNav(
    std::unique_ptr<UserEventSpecifics> specifics,
    int64_t navigation_id = 1) {
  specifics->set_navigation_id(navigation_id);
  return specifics;
}

void CreateAndFinalizeTrial(const std::string& trial,
                            const std::string& group) {
  // Must actually query for the group to finalize the field trial.
  EXPECT_EQ(group,
            FieldTrialList::CreateFieldTrial(trial, group)->group_name());
  // Notification is posted, which we need the service to pick up.
  base::RunLoop().RunUntilIdle();
}

class TestSyncService : public FakeSyncService {
 public:
  TestSyncService(bool is_engine_initialized,
                  bool is_using_secondary_passphrase,
                  ModelTypeSet preferred_data_types)
      : is_engine_initialized_(is_engine_initialized),
        is_using_secondary_passphrase_(is_using_secondary_passphrase),
        preferred_data_types_(preferred_data_types) {}

  bool IsEngineInitialized() const override { return is_engine_initialized_; }
  bool IsUsingSecondaryPassphrase() const override {
    return is_using_secondary_passphrase_;
  }

  ModelTypeSet GetPreferredDataTypes() const override {
    return preferred_data_types_;
  }

 private:
  bool is_engine_initialized_;
  bool is_using_secondary_passphrase_;
  ModelTypeSet preferred_data_types_;
};

class TestGlobalIdMapper : public GlobalIdMapper {
  void AddGlobalIdChangeObserver(GlobalIdChange callback) override {}
  int64_t GetLatestGlobalId(int64_t global_id) override { return global_id; }
};

class UserEventServiceImplTest : public testing::Test {
 protected:
  UserEventServiceImplTest()
      : sync_service_(true, false, {HISTORY_DELETE_DIRECTIVES}),
        field_trial_list_(nullptr) {}

  std::unique_ptr<UserEventSyncBridge> MakeBridge() {
    return std::make_unique<UserEventSyncBridge>(
        ModelTypeStoreTestUtil::FactoryForInMemoryStoreForTest(),
        RecordingModelTypeChangeProcessor::FactoryForBridgeTest(&processor_),
        &mapper_);
  }

  TestSyncService* sync_service() { return &sync_service_; }
  const RecordingModelTypeChangeProcessor& processor() { return *processor_; }

 private:
  TestSyncService sync_service_;
  RecordingModelTypeChangeProcessor* processor_;
  TestGlobalIdMapper mapper_;
  base::MessageLoop message_loop_;
  FieldTrialList field_trial_list_;
};

TEST_F(UserEventServiceImplTest, MightRecordEventsFeatureEnabled) {
  // All conditions are met, might record.
  EXPECT_TRUE(UserEventServiceImpl::MightRecordEvents(false, sync_service()));
  // No sync service, will not record.
  EXPECT_FALSE(UserEventServiceImpl::MightRecordEvents(false, nullptr));
  // Off the record, will not record.
  EXPECT_FALSE(UserEventServiceImpl::MightRecordEvents(true, sync_service()));
}

TEST_F(UserEventServiceImplTest, MightRecordEventsFeatureDisabled) {
  // Will not record because the default on feature is overridden.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(switches::kSyncUserEvents);
  EXPECT_FALSE(UserEventServiceImpl::MightRecordEvents(false, sync_service()));
}

TEST_F(UserEventServiceImplTest, ShouldRecord) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordNoHistory) {
  TestSyncService no_history_sync_service(true, false, ModelTypeSet());
  UserEventServiceImpl service(&no_history_sync_service, MakeBridge());

  // Only record events without navigation ids when history sync is off.
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordPassphrase) {
  TestSyncService passphrase_sync_service(true, true,
                                          {HISTORY_DELETE_DIRECTIVES});
  UserEventServiceImpl service(&passphrase_sync_service, MakeBridge());

  // Only record events without navigation ids when a passphrase is used.
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordEngineOff) {
  TestSyncService engine_not_initialized_sync_service(
      false, false, {HISTORY_DELETE_DIRECTIVES});
  UserEventServiceImpl service(&engine_not_initialized_sync_service,
                               MakeBridge());

  // Only record events without navigation ids when the engine is off.
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordEmpty) {
  UserEventServiceImpl service(sync_service(), MakeBridge());

  // All untyped events should always be ignored.
  service.RecordUserEvent(Event());
  EXPECT_EQ(0u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(Event()));
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecordHasNavigationId) {
  UserEventServiceImpl service(sync_service(), MakeBridge());

  // Verify logic for types that might or might not have a navigation id.
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(AsTest(Event())));
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Verify logic for types that must have a navigation id.
  service.RecordUserEvent(AsDetection(Event()));
  EXPECT_EQ(2u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(AsDetection(Event())));
  EXPECT_EQ(3u, processor().put_multimap().size());

  // Verify logic for types that cannot have a navigation id.
  service.RecordUserEvent(AsTrial(Event()));
  EXPECT_EQ(4u, processor().put_multimap().size());
  service.RecordUserEvent(WithNav(AsTrial(Event())));
  EXPECT_EQ(4u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, SessionIdIsDifferent) {
  UserEventServiceImpl service1(sync_service(), MakeBridge());
  service1.RecordUserEvent(AsTest(Event()));
  ASSERT_EQ(1u, processor().put_multimap().size());
  auto put1 = processor().put_multimap().begin();
  int64_t session_id1 = put1->second->specifics.user_event().session_id();

  UserEventServiceImpl service2(sync_service(), MakeBridge());
  service2.RecordUserEvent(AsTest(Event()));
  // The object processor() points to has changed to be |service2|'s processor.
  ASSERT_EQ(1u, processor().put_multimap().size());
  auto put2 = processor().put_multimap().begin();
  int64_t session_id2 = put2->second->specifics.user_event().session_id();

  EXPECT_NE(session_id1, session_id2);
}

TEST_F(UserEventServiceImplTest, FieldTrialOnGroupFinalization) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());

  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Being re-notified of the same finalization should not trigger re-emission.
  service.OnFieldTrialGroupFinalized(kTrial1, kGroup);
}

TEST_F(UserEventServiceImplTest, FieldTrialOnRecordUserEvent) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  EXPECT_EQ(0u, processor().put_multimap().size());

  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Subsequent records should only record themselves.
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(3u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialOnRegisterDependent) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());

  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Re-registering the same dependency shouldn't trigger a re-emission.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Neither should a different dependency that would emit the same trial.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kFieldTrialEvent,
                                      kTrial1);
  EXPECT_EQ(2u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialChecksDontFinalize) {
  // We avoid asking for the group, which would finalize. The service should be
  // careful to do the same.
  EXPECT_TRUE(FieldTrialList::CreateFieldTrial(kTrial1, kGroup));

  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);

  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());

  // Active and finalized are synonymous from our perspective.
  EXPECT_FALSE(FieldTrialList::IsTrialActive(kTrial1));
}

TEST_F(UserEventServiceImplTest, MultipleFieldTrials) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(AsTest(Event()));
  service.RecordUserEvent(WithNav(AsDetection(Event())));
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Both of these will be satisfied, should emit.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  service.RegisterDependentFieldTrial(
      UserEventSpecifics::kLanguageDetectionEvent, kTrial1);
  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(3u, processor().put_multimap().size());

  // One of these will be satisfied, should emit.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial2);
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTranslationEvent,
                                      kTrial2);
  CreateAndFinalizeTrial(kTrial2, kGroup);
  EXPECT_EQ(4u, processor().put_multimap().size());

  // Neither will be satisfied, should not emit.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTranslationEvent,
                                      kTrial3);
  service.RegisterDependentFieldTrial(
      UserEventSpecifics::kGaiaPasswordReuseEvent, kTrial3);
  EXPECT_EQ(4u, processor().put_multimap().size());
  CreateAndFinalizeTrial(kTrial3, kGroup);
}

TEST_F(UserEventServiceImplTest, FieldTrialFeatureDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      switches::kSyncUserFieldTrialEvents);

  EXPECT_TRUE(FieldTrialList::CreateFieldTrial(kTrial1, kGroup));
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialCircularDependency) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(UserEventSpecifics::kFieldTrialEvent,
                                      kTrial1);
  service.RecordUserEvent(AsTrial(Event()));
  EXPECT_EQ(2u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialOverMax) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  CreateAndFinalizeTrial(kTrial2, kGroup);
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      switches::kSyncUserFieldTrialEvents, {{"field_trial_max_count", "1"}});
  UserEventServiceImpl service(sync_service(), MakeBridge());

  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(1u, processor().put_multimap().size());

  // |kTrial1| will the be the first trial, and thus should be recorded.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Recording |kTrial2| would bring the count of trials over the max, so it
  // should be skipped instead.
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial2);
  EXPECT_EQ(2u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialTimer) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());

  {
    // Start with 0 delay, which should mean we post immediately to record.
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeatureWithParameters(
        switches::kSyncUserFieldTrialEvents,
        {{"field_trial_delay_seconds", "0"}});

    service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                        kTrial1);
    service.RecordUserEvent(AsTest(Event()));
    EXPECT_EQ(2u, processor().put_multimap().size());
  }

  // Now that |scoped_feature_list| is gone, we should reset to default,
  // otherwise our RunUntilIdle() would infinitively loop with a delay of 0.
  base::RunLoop().RunUntilIdle();
  // Should have picked up exactly 1 more FieldTrial event.
  EXPECT_EQ(3u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialBatched) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  CreateAndFinalizeTrial(kTrial2, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial1);
  service.RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent, kTrial2);

  // One is the test event, the other is the FieldTrial, which will contain both
  // trial names.
  service.RecordUserEvent(AsTest(Event()));
  EXPECT_EQ(2u, processor().put_multimap().size());
}

}  // namespace

}  // namespace syncer
