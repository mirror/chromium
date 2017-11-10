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

std::unique_ptr<UserEventSpecifics> TestEvent() {
  auto specifics = std::make_unique<UserEventSpecifics>();
  specifics->mutable_test_event();
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

TEST_F(UserEventServiceImplTest, ShouldNotRecordNoHistory) {
  TestSyncService no_history_sync_service(true, false, ModelTypeSet());
  UserEventServiceImpl service(&no_history_sync_service, MakeBridge());
  service.RecordUserEvent(std::make_unique<UserEventSpecifics>());
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldNotRecordPassphrase) {
  TestSyncService passphrase_sync_service(true, true,
                                          {HISTORY_DELETE_DIRECTIVES});
  UserEventServiceImpl service(&passphrase_sync_service, MakeBridge());
  service.RecordUserEvent(std::make_unique<UserEventSpecifics>());
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldNotRecordEngineOff) {
  TestSyncService engine_not_initialized_sync_service(
      false, false, {HISTORY_DELETE_DIRECTIVES});
  UserEventServiceImpl service(&engine_not_initialized_sync_service,
                               MakeBridge());
  service.RecordUserEvent(std::make_unique<UserEventSpecifics>());
  EXPECT_EQ(0u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, ShouldRecord) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(std::make_unique<UserEventSpecifics>());
  EXPECT_EQ(1u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, SessionIdIsDifferent) {
  UserEventServiceImpl service1(sync_service(), MakeBridge());
  service1.RecordUserEvent(std::make_unique<UserEventSpecifics>());
  ASSERT_EQ(1u, processor().put_multimap().size());
  auto put1 = processor().put_multimap().begin();
  int64_t session_id1 = put1->second->specifics.user_event().session_id();

  UserEventServiceImpl service2(sync_service(), MakeBridge());
  service2.RecordUserEvent(std::make_unique<UserEventSpecifics>());
  // The object processor() points to has changed to be |service2|'s processor.
  ASSERT_EQ(1u, processor().put_multimap().size());
  auto put2 = processor().put_multimap().begin();
  int64_t session_id2 = put2->second->specifics.user_event().session_id();

  EXPECT_NE(session_id1, session_id2);
}

TEST_F(UserEventServiceImplTest, FieldTrialOnGroupFinalization) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);
  service.RecordUserEvent(TestEvent());
  EXPECT_EQ(1u, processor().put_multimap().size());

  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Being re-notified of the same finalization should not trigger re-emission.
  service.OnFieldTrialGroupFinalized(kTrial1, kGroup);
}

TEST_F(UserEventServiceImplTest, FieldTrialOnRecordUserEvent) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);
  EXPECT_EQ(0u, processor().put_multimap().size());

  service.RecordUserEvent(TestEvent());
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Subsequent records should only record themselves.
  service.RecordUserEvent(TestEvent());
  EXPECT_EQ(3u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialOnRegisterDependent) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(TestEvent());
  EXPECT_EQ(1u, processor().put_multimap().size());

  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Re-registering the same dependency shouldn't trigger a re-emission.
  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Neither should a different dependency that would emit the same trial.
  service.RegisterDependentFieldTrial(kTrial1,
                                      UserEventSpecifics::kFieldTrialEvent);
  EXPECT_EQ(2u, processor().put_multimap().size());
}

TEST_F(UserEventServiceImplTest, FieldTrialChecksDontFinalize) {
  // We avoid asking for the group, which would finalize. The service should be
  // careful to do the same.
  EXPECT_TRUE(FieldTrialList::CreateFieldTrial(kTrial1, kGroup));

  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);

  service.RecordUserEvent(TestEvent());
  EXPECT_EQ(1u, processor().put_multimap().size());

  // Active and finalized are synonymous from our perspective.
  EXPECT_FALSE(FieldTrialList::IsTrialActive(kTrial1));
}

TEST_F(UserEventServiceImplTest, MultipleFieldTrials) {
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RecordUserEvent(TestEvent());
  auto detectionEvent = std::make_unique<UserEventSpecifics>();
  detectionEvent->mutable_language_detection_event();
  service.RecordUserEvent(std::move(detectionEvent));
  EXPECT_EQ(2u, processor().put_multimap().size());

  // Both of these will be satisfied, should emit.
  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);
  service.RegisterDependentFieldTrial(
      kTrial1, UserEventSpecifics::kLanguageDetectionEvent);
  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(3u, processor().put_multimap().size());

  // One of these will be satisfied, should emit.
  service.RegisterDependentFieldTrial(kTrial2, UserEventSpecifics::kTestEvent);
  service.RegisterDependentFieldTrial(kTrial2,
                                      UserEventSpecifics::kTranslationEvent);
  CreateAndFinalizeTrial(kTrial2, kGroup);
  EXPECT_EQ(4u, processor().put_multimap().size());

  // Neither will be satisfied, should not emit.
  service.RegisterDependentFieldTrial(kTrial3,
                                      UserEventSpecifics::kTranslationEvent);
  service.RegisterDependentFieldTrial(
      kTrial3, UserEventSpecifics::kGaiaPasswordReuseEvent);
  EXPECT_EQ(4u, processor().put_multimap().size());
  CreateAndFinalizeTrial(kTrial3, kGroup);
}

TEST_F(UserEventServiceImplTest, FieldTrialFeatureDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      switches::kSyncUserFieldTrialEvents);

  EXPECT_TRUE(FieldTrialList::CreateFieldTrial(kTrial1, kGroup));
  UserEventServiceImpl service(sync_service(), MakeBridge());
  service.RegisterDependentFieldTrial(kTrial1, UserEventSpecifics::kTestEvent);
  service.RecordUserEvent(TestEvent());
  EXPECT_EQ(1u, processor().put_multimap().size());
}

}  // namespace

}  // namespace syncer
