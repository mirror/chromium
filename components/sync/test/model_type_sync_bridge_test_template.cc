// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/model_type_sync_bridge_test_template.h"

#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/mock_model_type_change_processor.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/model/sync_metadata_store.h"
#include "components/sync/test/proto_matchers.h"
#include "components/sync/protocol/proto_visitors.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {
namespace {

using testing::ElementsAre;
using testing::Eq;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::IsEmpty;
using testing::IsNull;
using testing::Matcher;
using testing::Not;
using testing::Pair;
using testing::UnorderedElementsAre;
using testing::Return;
using testing::SaveArg;
using testing::SizeIs;
using testing::WithArg;
using testing::_;

syncer::EntityDataPtr SpecificsToEntity(
    const sync_pb::EntitySpecifics& specifics) {
  syncer::EntityData data;
  data.client_tag_hash = "ignored";
  data.specifics = specifics;
  return data.PassToPtr();
}

// |matcher| should be of type Matcher<EntityMetadataMap>.
MATCHER_P(MetadataBatchContains, matcher, "") {
  MetadataBatch* batch = arg;
  if (batch == nullptr) {
    *result_listener << "which is null";
    return false;
  }
  EntityMetadataMap metadata_map = batch->TakeAllMetadata();
  return ExplainMatchResult(Matcher<EntityMetadataMap>(matcher), metadata_map,
                            result_listener);
}

Matcher<const EntityData&> HasSpecifics(
    const sync_pb::EntitySpecifics& expected) {
  return testing::Field(&EntityData::specifics, EqualsProto(expected));
}

Matcher<EntityData*> PointeeHasSpecifics(
    const sync_pb::EntitySpecifics& expected) {
  return testing::Pointee(HasSpecifics(expected));
}

// A future is a blocking closure (in nested runloop) that returns a value T.
// Wrapped in a dedicated class for readability.
template <typename T>
class Future {
 public:
  T WaitForResult() { return callback.Run(); }

  base::Callback<T()> callback;
};

// std::promise or future-like object that receives input from a callback.
template <typename T>
class CallbackWaiter {
 public:
  using value_type =
      typename std::remove_const<typename std::remove_reference<T>::type>::type;

  base::Callback<void(T)> CallbackToWaitFor() {
    CHECK(!state_);
    state_ = std::make_unique<State>();
    return base::Bind(&State::Set, state_->AsWeakPtr());
  }

  Future<value_type> GetFuture() {
    CHECK(state_);
    Future<value_type> future;
    future.callback = base::BindRepeating(&State::Wait, std::move(state_));
    return future;
  }

 private:
  class State : public base::SupportsWeakPtr<State> {
   public:
    void Set(T value) {
      result = std::move(value);
      if (!notify_closure.is_null()) {
        std::move(notify_closure).Run();
      }
    }

    value_type Wait() {
      if (!result.has_value()) {
        base::RunLoop loop;
        notify_closure = loop.QuitClosure();
        loop.Run();
        CHECK(result.has_value());
      }
      return std::move(*result);
    }

    base::Optional<value_type> result;
    base::OnceClosure notify_closure;
  };

  std::unique_ptr<State> state_;
};

Future<std::unique_ptr<MetadataBatch>> ExpectCallToModelReadyToSync(
    MockModelTypeChangeProcessor* processor) {
  CallbackWaiter<std::unique_ptr<MetadataBatch>> waiter;
  auto cb = waiter.CallbackToWaitFor();
  // Forward calls to ModelReadyToSync() to the waiter object.
  ON_CALL(*processor, DoModelReadyToSync(_))
      .WillByDefault(Invoke([cb](MetadataBatch* batch) {
        cb.Run(std::make_unique<MetadataBatch>(std::move(*batch)));
      }));
  return waiter.GetFuture();
}

}  // namespace

void PrintTo(const EntityData& entity_data, ::std::ostream* os) {
  *os << "entity data with specifics ";
  ProtoPrinter::Print(entity_data.specifics, os);
}

ModelTypeSyncBridgeTester::LocalType::~LocalType() = default;

ModelTypeSyncBridgeTester::TestEntityData::TestEntityData(
    std::unique_ptr<LocalType> local,
    const sync_pb::EntitySpecifics& specifics)
    : local(std::move(local)), specifics(specifics) {}

ModelTypeSyncBridgeTester::TestEntityData::TestEntityData(TestEntityData&&) =
    default;

ModelTypeSyncBridgeTester::TestEntityData::~TestEntityData() = default;

ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey::
    TestEntityDataWithStorageKey(TestEntityData data,
                                 const std::string& storage_key)
    : storage_key(storage_key),
      local(std::move(data.local)),
      specifics(data.specifics) {}

ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey::
    TestEntityDataWithStorageKey(TestEntityDataWithStorageKey&&) = default;

ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey::
    ~TestEntityDataWithStorageKey() = default;

ModelTypeSyncBridgeTester::ConflictResolution::ConflictResolution(
    std::unique_ptr<LocalType> local,
    const sync_pb::EntitySpecifics& remote_specifics,
    const sync_pb::EntitySpecifics& expected_result)
    : local(std::move(local)),
      remote_specifics(remote_specifics),
      expected_result(expected_result) {}

ModelTypeSyncBridgeTester::ConflictResolution::ConflictResolution(
    ConflictResolution&& other)
    : local(std::move(other.local)) {
  remote_specifics.Swap(&other.remote_specifics);
  expected_result.Swap(&other.expected_result);
}

ModelTypeSyncBridgeTester::ConflictResolution::~ConflictResolution() = default;

ModelTypeSyncBridgeTester::ModelTypeSyncBridgeTester() = default;

ModelTypeSyncBridgeTester::~ModelTypeSyncBridgeTester() = default;

std::map<std::string, sync_pb::EntitySpecifics>
ModelTypeSyncBridgeTester::GetFixedLocalEntities() {
  return {};
}

std::vector<ModelTypeSyncBridgeTester::ConflictResolution>
ModelTypeSyncBridgeTester::GetConflictResolutionTests() {
  std::vector<ConflictResolution> tests;
  TestEntityData entity = std::move(GetTestEntities()[0]);
  sync_pb::EntitySpecifics mutated_specifics = entity.specifics;
  MutateTestEntitySpecifics(&mutated_specifics);

  // By default, server-side wins.
  tests.emplace_back(std::move(entity.local), mutated_specifics,
                     mutated_specifics);
  return tests;
}

ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey
ModelTypeSyncBridgeTester::CreateAndStoreLocalTestEntity() {
  TestEntityData entity = std::move(GetTestEntities()[0]);
  std::string storage_key = StoreEntityInLocalModel(entity.local.get());
  return TestEntityDataWithStorageKey(std::move(entity), storage_key);
}

std::map<std::string, sync_pb::EntitySpecifics>
ModelTypeSyncBridgeTester::GetAllData() {
  CallbackWaiter<std::unique_ptr<DataBatch>> waiter;
  bridge()->GetAllData(waiter.CallbackToWaitFor());

  std::unique_ptr<DataBatch> batch = waiter.GetFuture().WaitForResult();
  EXPECT_NE(nullptr, batch);

  std::map<std::string, sync_pb::EntitySpecifics> storage_key_to_specifics;
  if (batch != nullptr) {
    while (batch->HasNext()) {
      const KeyAndData& pair = batch->Next();
      storage_key_to_specifics[pair.first] = pair.second->specifics;
    }
  }
  return storage_key_to_specifics;
}

sync_pb::EntitySpecifics ModelTypeSyncBridgeTester::GetData(
    const std::string& storage_key) {
  CallbackWaiter<std::unique_ptr<DataBatch>> waiter;
  bridge()->GetData({storage_key}, waiter.CallbackToWaitFor());

  std::unique_ptr<DataBatch> batch = waiter.GetFuture().WaitForResult();
  EXPECT_NE(nullptr, batch);

  sync_pb::EntitySpecifics specifics;
  if (batch != nullptr && batch->HasNext()) {
    const KeyAndData& pair = batch->Next();
    specifics = pair.second->specifics;
    EXPECT_FALSE(batch->HasNext());
  }
  return specifics;
}

bool ModelTypeSyncBridgeTester::CanModifyLocalModel() {
  return GetFixedLocalEntities().empty();
}

SyncBridgeTest::SyncBridgeTest() {
  // Even if we use NiceMock, let's be strict about errors and let tests
  // explicitly list them.
  EXPECT_CALL(processor_, ReportError(_, _)).Times(0);
}

SyncBridgeTest::~SyncBridgeTest() {}

ModelTypeSyncBridgeTester* SyncBridgeTest::tester() {
  CHECK(tester_) << "Bridge not initialized";
  return tester_.get();
}

ModelTypeSyncBridge* SyncBridgeTest::bridge() {
  ModelTypeSyncBridge* bridge = tester()->bridge();
  CHECK(bridge);
  return bridge;
}

MockModelTypeChangeProcessor& SyncBridgeTest::processor() {
  return processor_;
}

std::unique_ptr<MetadataBatch>
SyncBridgeTest::InitializeBridgeAndWaitUntilReady() {
  auto future = ExpectCallToModelReadyToSync(&processor());
  // Initialize the tester (which includes the bridge).
  tester_ = GetParam().Run(processor_.FactoryForBridgeTest());
  std::unique_ptr<MetadataBatch> metadata_batch = future.WaitForResult();

  // The processor is now tracking metadata.
  ON_CALL(processor(), IsTrackingMetadata()).WillByDefault(Return(true));
  // Cause all calls to Put() to reflect as metadata change. This reflects what
  // processors usually do once the initial sync is done.
  ON_CALL(processor(), DoPut(_, _, _))
      .WillByDefault(Invoke([](const std::string& storage_key, EntityData* data,
                               MetadataChangeList* metadata_change_list) {
        metadata_change_list->UpdateMetadata(storage_key,
                                             sync_pb::EntityMetadata());
      }));

  return metadata_batch;
}

std::unique_ptr<MetadataBatch>
SyncBridgeTest::RestartBridgeAndWaitUntilReady() {
  CHECK(tester_);
  auto future = ExpectCallToModelReadyToSync(&processor());
  tester_->Restart();
  return future.WaitForResult();
}

void SyncBridgeTest::StartSyncing(
    const std::vector<sync_pb::EntitySpecifics> initial_data) {
  EntityChangeList entity_change_list;
  for (const sync_pb::EntitySpecifics& specifics : initial_data) {
    EntityDataPtr entity_data = SpecificsToEntity(specifics);
    std::string storage_key;
    if (bridge()->SupportsGetStorageKey()) {
      storage_key = bridge()->GetStorageKey(entity_data.value());
    }
    entity_change_list.push_back(
        EntityChange::CreateAdd(storage_key, entity_data));
  }
  base::Optional<ModelError> error = bridge()->MergeSyncData(
      bridge()->CreateMetadataChangeList(), entity_change_list);
  EXPECT_FALSE(error) << error->ToString();
}

std::map<std::string, sync_pb::EntitySpecifics>
SyncBridgeTest::GetAllDataFromBridge() {
  return tester()->GetAllData();
}

// Test that prevents datatypes from accidentally returning the wrong value for
// SupportsGetStorageKey().
TEST_P(SyncBridgeTest, SupportsGetStorageKey) {
  InitializeBridgeAndWaitUntilReady();
  for (const auto& entity : tester()->GetTestEntities()) {
    if (bridge()->SupportsGetStorageKey()) {
      EXPECT_NE("", bridge()->GetStorageKey(
                        SpecificsToEntity(entity.specifics).value()));
    }
  }
}

TEST_P(SyncBridgeTest, InitWithEmptyModel) {
  EXPECT_CALL(processor(),
              DoModelReadyToSync(MetadataBatchContains(IsEmpty())));
  InitializeBridgeAndWaitUntilReady();
}

TEST_P(SyncBridgeTest, InitWithNonEmptyModel) {
  InitializeBridgeAndWaitUntilReady();

  const auto entity = tester()->CreateAndStoreLocalTestEntity();
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(processor(),
              DoModelReadyToSync(MetadataBatchContains(SizeIs(1))));
  RestartBridgeAndWaitUntilReady();
}

TEST_P(SyncBridgeTest, GetAllDataWithEmptyModel) {
  InitializeBridgeAndWaitUntilReady();
  std::vector<Matcher<std::pair<const std::string, sync_pb::EntitySpecifics>>>
      expected;
  for (const auto& storage_key_and_specifics :
       tester()->GetFixedLocalEntities()) {
    expected.push_back(Pair(storage_key_and_specifics.first,
                            EqualsProto(storage_key_and_specifics.second)));
  }
  EXPECT_THAT(GetAllDataFromBridge(), testing::ElementsAreArray(expected));
}

// Add two typed urls locally and verify bridge can get them from GetAllData.
TEST_P(SyncBridgeTest, GetAllData) {
  InitializeBridgeAndWaitUntilReady();

  std::map<std::string, sync_pb::EntitySpecifics> storage_key_to_specifics;
  for (auto& test_entity : tester()->GetTestEntities()) {
    std::string storage_key =
        tester()->StoreEntityInLocalModel(test_entity.local.get());
    storage_key_to_specifics[storage_key] = test_entity.specifics;
  }
  CHECK(!storage_key_to_specifics.empty());

  std::vector<Matcher<std::pair<const std::string, sync_pb::EntitySpecifics>>>
      expected;
  for (const auto& entry : storage_key_to_specifics) {
    expected.push_back(Pair(entry.first, EqualsProto(entry.second)));
  }

  EXPECT_THAT(GetAllDataFromBridge(), testing::ElementsAreArray(expected));

  // After restart, we expect the data to remain unchanged.
  RestartBridgeAndWaitUntilReady();
  EXPECT_THAT(GetAllDataFromBridge(), testing::ElementsAreArray(expected));
}

// Add two typed urls locally and verify bridge can get them from GetData.
TEST_P(SyncBridgeTest, GetData) {
  InitializeBridgeAndWaitUntilReady();

  const auto entity = tester()->CreateAndStoreLocalTestEntity();

  EXPECT_THAT(tester()->GetData(entity.storage_key),
              EqualsProto(entity.specifics));
  EXPECT_THAT(tester()->GetData("unknown1"),
              EqualsProto(sync_pb::EntitySpecifics()));
}

TEST_P(SyncBridgeTest, DisableSync) {
  InitializeBridgeAndWaitUntilReady();
  StartSyncing({});

  EXPECT_CALL(processor(), DisableSync());
  bridge()->DisableSync();

  // In the current implementaton, it is assumed that a processor always exists.
  // This can be changed, but care must be taken.
  EXPECT_THAT(bridge()->change_processor(), Not(IsNull()));
}

TEST_P(SyncBridgeTest, MergeEmpty) {
  InitializeBridgeAndWaitUntilReady();

  EXPECT_CALL(processor(), DoPut(_, _, _))
      .Times(tester()->GetFixedLocalEntities().size());
  EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);

  StartSyncing({});

  EXPECT_THAT(GetAllDataFromBridge(),
              SizeIs(tester()->GetFixedLocalEntities().size()));
}

// Starting sync with no local data should just push the synced url into the
// backend.
TEST_P(SyncBridgeTest, MergeWithoutLocal) {  // MergeUrlEmptyLocal) {
  InitializeBridgeAndWaitUntilReady();

  EXPECT_CALL(processor(), DoPut(_, _, _))
      .Times(tester()->GetFixedLocalEntities().size());

  const auto entity = std::move(tester()->GetTestEntities().back());
  std::string storage_key;
  if (bridge()->SupportsGetStorageKey()) {
    storage_key =
        bridge()->GetStorageKey(SpecificsToEntity(entity.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(),
                UpdateStorageKey(HasSpecifics(entity.specifics), _, _))
        .WillOnce(SaveArg<1>(&storage_key));
  }

  StartSyncing({entity.specifics});

  // Check that the backend was updated correctly.
  EXPECT_THAT(tester()->GetData(storage_key), EqualsProto(entity.specifics));
}

// Starting sync with no sync data should just push the local url to sync.
TEST_P(SyncBridgeTest, MergeWithoutRemote) {  // MergeUrlEmptySync
  InitializeBridgeAndWaitUntilReady();
  const auto entity = tester()->CreateAndStoreLocalTestEntity();

  EXPECT_CALL(processor(), DoPut(entity.storage_key,
                                 PointeeHasSpecifics(entity.specifics), _));

  StartSyncing({});

  // Check that the local cache was is still correct.
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity.storage_key, EqualsProto(entity.specifics))));
}

// Add a typed url locally and one to sync with the same data. Starting sync
// should result in no changes.
TEST_P(SyncBridgeTest, MergeIdentical) {  // MergeUrlNoChange
  InitializeBridgeAndWaitUntilReady();
  const auto entity = tester()->CreateAndStoreLocalTestEntity();

  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  std::string storage_key;
  if (bridge()->SupportsGetStorageKey()) {
    storage_key =
        bridge()->GetStorageKey(SpecificsToEntity(entity.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(),
                UpdateStorageKey(HasSpecifics(entity.specifics), _, _))
        .WillOnce(SaveArg<1>(&storage_key));
  }

  // Create the same data in sync.
  StartSyncing({entity.specifics});

  // Check that the local cache was is still correct.
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity.storage_key, EqualsProto(entity.specifics))));
}

TEST_P(SyncBridgeTest, MergeConflicts) {  // SimpleMerge
  InitializeBridgeAndWaitUntilReady();

  for (ModelTypeSyncBridgeTester::ConflictResolution& test :
       tester()->GetConflictResolutionTests()) {
    std::string storage_key =
        tester()->StoreEntityInLocalModel(test.local.get());

    testing::InSequence seq;

    if (bridge()->SupportsGetStorageKey()) {
      EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
    } else {
      // TODO(mastiz): There is a comment in typed_url_sync_bridge.cc that says:
      // "otherwise processor will have duplicate entries". That probably means
      // we should verify such a requirement here.
      EXPECT_CALL(
          processor(),
          UpdateStorageKey(HasSpecifics(test.expected_result), storage_key, _));
    }

    // We expect zero or one call to Put(). If a call exists, it must contain
    // the expected storage key and specifics.
    EXPECT_CALL(
        processor(),
        DoPut(storage_key, PointeeHasSpecifics(test.expected_result), _))
        .Times(testing::AtMost(1));

    StartSyncing({test.remote_specifics});
    testing::Mock::VerifyAndClearExpectations(&processor());
    // Restart the bridge to avoid collions across tests.
    RestartBridgeAndWaitUntilReady();
  }
}

// Create a local typed URL with one TYPED visit after sync has started. Check
// that sync is sent an ADD change for the new URL.
TEST_P(SyncBridgeTest, LocalAddAfterSyncStarted) {  // AddLocalTypedUrl
  InitializeBridgeAndWaitUntilReady();
  // **** THIS ONE WOULD BE DOABLE IF WE MIGRATED TO WORKER ********
  if (!tester()->CanModifyLocalModel()) {
    return;
  }
  StartSyncing({});

  const auto entity = std::move(tester()->GetTestEntities()[0]);
  // We don't yet have information about the expectations, so we need to store
  // the arguments to have them verified later.
  std::string put_storage_key;
  base::RunLoop loop;
  EXPECT_CALL(processor(), DoPut(_, PointeeHasSpecifics(entity.specifics), _))
      .WillRepeatedly(WithArg<0>(Invoke([&](const std::string& storage_key) {
        put_storage_key = storage_key;
        loop.Quit();
      })));
  // Create a local entity that is not already in sync. Wait until Put() is
  // received.
  const std::string storage_key =
      tester()->StoreEntityInLocalModel(entity.local.get());
  loop.Run();
  EXPECT_THAT(put_storage_key, Eq(storage_key));
}

// Update a local typed URL that is already synced. Check that sync is sent an
// UPDATE for the existing url,
TEST_P(SyncBridgeTest, LocalUpdateForSyncedEntity) {  // UpdateLocalTypedUrl) {
  InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity = tester()->CreateAndStoreLocalTestEntity();
  StartSyncing({entity.specifics});
  ASSERT_THAT(tester()->GetData(entity.storage_key),
              EqualsProto(entity.specifics));

  // Mutating the local entity (below) should cause a call to Put().
  sync_pb::EntitySpecifics mutated_specifics1 = entity.specifics;
  tester()->MutateTestEntitySpecifics(&mutated_specifics1);
  ASSERT_THAT(mutated_specifics1, Not(EqualsProto(entity.specifics)));

  base::RunLoop loop;
  EXPECT_CALL(processor(), DoPut(entity.storage_key,
                                 PointeeHasSpecifics(mutated_specifics1), _))
      .WillOnce(InvokeWithoutArgs(&loop, &base::RunLoop::Quit));
  tester()->MutateStoredTestEntity(entity.storage_key, entity.local.get());
  loop.Run();
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity.storage_key, EqualsProto(mutated_specifics1))));
}

// Locally delete an entity which is already synced. Check that the processor
// receives a call to Delete().
TEST_P(SyncBridgeTest, LocalDeletionSyncedEntity) {
  InitializeBridgeAndWaitUntilReady();
  if (!tester()->CanModifyLocalModel()) {
    return;
  }

  // Create the same data locally and in sync.
  const auto entity = tester()->CreateAndStoreLocalTestEntity();
  StartSyncing({entity.specifics});
  ASSERT_THAT(tester()->GetData(entity.storage_key),
              EqualsProto(entity.specifics));

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  EXPECT_CALL(processor(), Delete(entity.storage_key, _));

  tester()->DeleteEntityFromLocalModel(entity.storage_key);
  EXPECT_THAT(tester()->GetData(entity.storage_key),
              EqualsProto(sync_pb::EntitySpecifics()));

  // Verify that the entity is not resurrected after restart.
  RestartBridgeAndWaitUntilReady();
  EXPECT_THAT(tester()->GetData(entity.storage_key),
              EqualsProto(sync_pb::EntitySpecifics()));
}

// Create a remote typed URL and visit, then send to sync bridge after sync
// has started. Check that local DB is received the new URL and visit.
TEST_P(SyncBridgeTest, RemoteAddAfterSyncStarted) {  // AddUrlAndVisits) {
  InitializeBridgeAndWaitUntilReady();
  StartSyncing({});

  const auto entity = std::move(tester()->GetTestEntities()[0]);

  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  std::string storage_key;
  if (bridge()->SupportsGetStorageKey()) {
    storage_key =
        bridge()->GetStorageKey(SpecificsToEntity(entity.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    // TODO(mastiz): How can we verify a storage_key here? The old tests
    // compared it to IntToStorageKey(1), but that seems overly strict.
    EXPECT_CALL(processor(),
                UpdateStorageKey(HasSpecifics(entity.specifics), _, _))
        .WillOnce(SaveArg<1>(&storage_key));
  }

  base::Optional<ModelError> error = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateAdd(storage_key,
                               SpecificsToEntity(entity.specifics))});
  EXPECT_FALSE(error) << error->ToString();

  testing::Mock::VerifyAndClearExpectations(&processor());
  EXPECT_THAT(storage_key, Not(IsEmpty()));
  EXPECT_THAT(tester()->GetData(storage_key), EqualsProto(entity.specifics));

  // Verify that the entity remains there after restart.
  RestartBridgeAndWaitUntilReady();
  EXPECT_THAT(tester()->GetData(storage_key), EqualsProto(entity.specifics));
}

// Update a remote typed URL and create a new visit that is already synced, then
// send the update to sync bridge. Check that local DB is received an
// UPDATE for the existing url and new visit.
TEST_P(SyncBridgeTest, RemoteUpdateForSyncedEntity) {  // UpdateUrlAndVisits) {
  InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity = std::move(tester()->GetTestEntities().back());
  std::string storage_key =
      tester()->StoreEntityInLocalModel(entity.local.get());
  StartSyncing({entity.specifics});
  ASSERT_THAT(tester()->GetData(storage_key), EqualsProto(entity.specifics));

  sync_pb::EntitySpecifics mutated_specifics = entity.specifics;
  tester()->MutateTestEntitySpecifics(&mutated_specifics);
  ASSERT_THAT(mutated_specifics, Not(EqualsProto(entity.specifics)));

  // Remote update (below) should trigger no Put().
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);
  EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  base::Optional<ModelError> error = bridge()->ApplySyncChanges(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateUpdate(storage_key,
                                  SpecificsToEntity(mutated_specifics))});
  EXPECT_FALSE(error) << error->ToString();
  EXPECT_THAT(tester()->GetData(storage_key), EqualsProto(mutated_specifics));
}

TEST_P(SyncBridgeTest, RemoteUpdateDuringInitialMerge) {
  // TODO / DONOTSUBMIT: Is this a legit scenario, or is the processor
  // responsible for deferring the update until MergeSyncData() is finished?
}

// Delete a typed urls which already synced. Check that local DB receives the
// DELETE changes.
TEST_P(SyncBridgeTest,
       RemoteDeletionForSyncedEntity) {  // DeleteUrlAndVisits) {
  InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity = std::move(tester()->GetTestEntities().back());
  std::string storage_key =
      tester()->StoreEntityInLocalModel(entity.local.get());
  StartSyncing({entity.specifics});
  ASSERT_THAT(tester()->GetData(storage_key), EqualsProto(entity.specifics));

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_CALL(processor(), Delete(_, _)).Times(0);
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  // Remote deletion.
  base::Optional<ModelError> error =
      bridge()->ApplySyncChanges(bridge()->CreateMetadataChangeList(),
                                 {EntityChange::CreateDelete(storage_key)});
  EXPECT_FALSE(error) << error->ToString();

  EXPECT_THAT(tester()->GetData(storage_key),
              EqualsProto(sync_pb::EntitySpecifics()));
}

TEST_P(SyncBridgeTest,
       ModelTypeStateAfterRestart) {  // ApplySyncChangesStore) {
  InitializeBridgeAndWaitUntilReady();

  sync_pb::ModelTypeState state;
  state.set_encryption_key_name("ekn");

  std::unique_ptr<MetadataChangeList> metadata_changes =
      bridge()->CreateMetadataChangeList();
  metadata_changes->UpdateModelTypeState(state);
  base::Optional<ModelError> error =
      bridge()->ApplySyncChanges(std::move(metadata_changes), {});
  EXPECT_FALSE(error) << error->ToString();

  std::unique_ptr<MetadataBatch> metadata_after_restart =
      RestartBridgeAndWaitUntilReady();
  ASSERT_NE(nullptr, metadata_after_restart);
  EXPECT_THAT(metadata_after_restart->GetModelTypeState(), EqualsProto(state));
}

}  // namespace syncer
