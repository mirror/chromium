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
#include "components/sync/protocol/proto_visitors.h"
#include "components/sync/protocol/sync.pb.h"

namespace {

using testing::ElementsAre;
using testing::Eq;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::IsEmpty;
using testing::IsNull;
using testing::Not;
using testing::Pair;
using testing::UnorderedElementsAre;
using testing::Return;
using testing::SaveArg;
using testing::WithArg;
using testing::_;

syncer::EntityDataPtr SpecificsToEntity(
    const sync_pb::EntitySpecifics& specifics) {
  syncer::EntityData data;
  data.client_tag_hash = "ignored";
  data.specifics = specifics;
  return data.PassToPtr();
}

class ProtoPrinter {
 public:
  ProtoPrinter(std::ostream* os, int indentation)
      : os_(os), indentation_(indentation), indent_str_(indentation * 2, ' ') {}

  template <class P>
  void VisitBytes(const P& parent_proto,
                  const char* field_name,
                  const std::string& field) {
    std::string value;
    for (char c : field) {
      base::StringAppendF(&value, "\\u%04X ", c);
    }
    *os_ << indent_str_ << field_name << ": " << value << "\n";
  }

  // Types derived from MessageLite (i.e. protos).
  template <class P, class F>
  typename std::enable_if<
      std::is_base_of<google::protobuf::MessageLite, F>::value>::type
  Visit(const P&, const char* field_name, const F& field) {
    *os_ << indent_str_ << field_name << " {\n";
    ProtoPrinter visitor(os_, indentation_ + 1);
    ::syncer::VisitProtoFields(visitor, field);
    *os_ << indent_str_ << "}\n";
  }

  // RepeatedPtrField.
  template <class P, class F>
  void Visit(const P& parent_proto,
             const char* field_name,
             const google::protobuf::RepeatedPtrField<F>& fields) {
    for (const auto& field : fields) {
      Visit(parent_proto, field_name, field);
    }
  }

  // RepeatedField.
  template <class P, class F>
  void Visit(const P& parent_proto,
             const char* field_name,
             const google::protobuf::RepeatedField<F>& fields) {
    for (const auto& field : fields) {
      Visit(parent_proto, field_name, field);
    }
  }

  template <class P, class E>
  void VisitEnum(const P& parent_proto, const char* field_name, E field) {
    Visit(parent_proto, field_name, field);
  }

  // std::string.
  template <class P>
  void Visit(const P&, const char* field_name, const std::string& field) {
    *os_ << indent_str_ << field_name << ": \"" << field << "\"\n";
  }

  // Default implementation.
  template <class P, class F>
  typename std::enable_if<
      !std::is_base_of<google::protobuf::MessageLite, F>::value>::type
  Visit(const P&, const char* field_name, F field) {
    *os_ << indent_str_ << field_name << ": " << field << "\n";
  }

 private:
  std::ostream* os_;
  const int indentation_;
  const std::string indent_str_;
};

}  // namespace

namespace sync_pb {

// TODO(mastiz): I haven't managed to use a function template to support all
// sync protobug types, so we might need to list them all here for
// human-readable error messages.
inline void PrintTo(const TypedUrlSpecifics& proto, ::std::ostream* os) {
  *os << "{\n";
  ProtoPrinter visitor(os, /*indentation=*/1);
  ::syncer::VisitProtoFields(visitor, proto);
  *os << "}";
}

inline void PrintTo(const DeviceInfoSpecifics& proto, ::std::ostream* os) {
  *os << "{\n";
  ProtoPrinter visitor(os, /*indentation=*/1);
  ::syncer::VisitProtoFields(visitor, proto);
  *os << "}";
}

inline void PrintTo(const ReadingListSpecifics& proto, ::std::ostream* os) {
  *os << "{\n";
  ProtoPrinter visitor(os, /*indentation=*/1);
  ::syncer::VisitProtoFields(visitor, proto);
  *os << "}";
}

inline void PrintTo(const EntitySpecifics& proto, ::std::ostream* os) {
  *os << "{\n";
  ProtoPrinter visitor(os, /*indentation=*/1);
  ::syncer::VisitProtoFields(visitor, proto);
  *os << "}";
}

}  // namespace sync_pb

namespace syncer {

inline void PrintTo(const EntityData& entity_data, ::std::ostream* os) {
  *os << "entity data with specifics ";
  PrintTo(entity_data.specifics, os);
}

MATCHER_P(EqualsProto, expected, "") {
  // Unfortunately, we only have MessageLite protocol buffers in Chrome, so
  // comparing via DebugString() or MessageDifferencer is not working.
  // So we either need to compare field-by-field (maintenance heavy) or
  // compare the binary version (unusable diagnostic). Deciding for the latter.
  std::string expected_serialized, actual_serialized;
  expected.SerializeToString(&expected_serialized);
  arg.SerializeToString(&actual_serialized);
  return expected_serialized == actual_serialized;
}

testing::Matcher<const EntityData&> HasSpecifics(
    const sync_pb::EntitySpecifics& expected) {
  return testing::Field(&EntityData::specifics, EqualsProto(expected));
}

testing::Matcher<EntityData*> PointeeHasSpecifics(
    const sync_pb::EntitySpecifics& expected) {
  return testing::Pointee(HasSpecifics(expected));
}

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

  SyncBridgeTest::Future<value_type> GetFuture() {
    CHECK(state_);
    return SyncBridgeTest::Future<value_type>(
        base::BindRepeating(&State::Wait, std::move(state_)));
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

ModelTypeSyncBridgeTester::ModelTypeSyncBridgeTester() = default;

ModelTypeSyncBridgeTester::~ModelTypeSyncBridgeTester() = default;

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

std::vector<ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey>
ModelTypeSyncBridgeTester::HasFixedLocalEntities() {
  return {};
}

std::vector<ModelTypeSyncBridgeTester::ConflictResolution>
ModelTypeSyncBridgeTester::GetConflictResolutionTests() const {
  std::vector<ConflictResolution> tests;
  CHECK(false);
  /*
  tests.push_back({std::move(GetTestEntity1().local),
          std::make_unique<sync_pb::EntitySpecifics>(
                         GetMutatedTestEntity1Specifics()),
                     std::make_unique<sync_pb::EntitySpecifics>(
                         GetMutatedTestEntity1Specifics())});
  */
  return tests;
}

ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey
ModelTypeSyncBridgeTester::CreateAndStoreLocalTestEntity1() {
  TestEntityData entity = GetTestEntity1();
  std::string storage_key = StoreEntityInLocalModel(entity.local.get());
  return TestEntityDataWithStorageKey(std::move(entity), storage_key);
}

ModelTypeSyncBridgeTester::TestEntityDataWithStorageKey
ModelTypeSyncBridgeTester::CreateAndStoreLocalTestEntity2() {
  TestEntityData entity = GetTestEntity2();
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

SyncBridgeTest::SyncBridgeTest() {
  // Even if we use NiceMock, let's be strict about errors and let tests
  // explicitly list them.
  EXPECT_CALL(processor_, ReportError(_, _)).Times(0);

  ON_CALL(processor_, IsTrackingMetadata()).WillByDefault(Return(true));
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
};

SyncBridgeTest::Future<std::unique_ptr<MetadataBatch>>
SyncBridgeTest::InitializeBridge() {
  auto future = GetModelReadyToSyncFuture();
  // Initialize the tester (which includes the bridge).
  tester_ = GetParam().Run(
      MockModelTypeChangeProcessor::FactoryForBridgeTest(&processor_));
  return future;
}

std::unique_ptr<MetadataBatch>
SyncBridgeTest::InitializeBridgeAndWaitUntilReady() {
  return InitializeBridge().WaitForResult();
}

std::unique_ptr<MetadataBatch>
SyncBridgeTest::RestartBridgeAndWaitUntilReady() {
  CHECK(tester_);
  auto future = GetModelReadyToSyncFuture();
  ModelTypeSyncBridgeTester* tester = tester_.get();
  tester_ = tester->Restart(std::move(tester_));
  return future.WaitForResult();
}

std::map<std::string, sync_pb::EntitySpecifics>
SyncBridgeTest::GetAllDataFromBridge() {
  return tester()->GetAllData();
}

SyncBridgeTest::Future<base::Optional<ModelError>> SyncBridgeTest::StartSyncing(
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

  CallbackWaiter<const base::Optional<ModelError>&> waiter;
  bridge()->MergeSyncData2(bridge()->CreateMetadataChangeList(),
                           std::move(entity_change_list),
                           waiter.CallbackToWaitFor());
  return waiter.GetFuture();
}

void SyncBridgeTest::StartSyncingAndWait(
    const std::vector<sync_pb::EntitySpecifics> initial_data) {
  base::Optional<ModelError> error = StartSyncing(initial_data).WaitForResult();
  EXPECT_FALSE(error) << error->ToString();
}

SyncBridgeTest::Future<base::Optional<ModelError>>
SyncBridgeTest::ApplySyncChanges(
    std::unique_ptr<MetadataChangeList> metadata_change_list,
    EntityChangeList entity_changes) {
  CallbackWaiter<const base::Optional<ModelError>&> waiter;
  bridge()->ApplySyncChanges2(std::move(metadata_change_list),
                              std::move(entity_changes),
                              waiter.CallbackToWaitFor());
  return waiter.GetFuture();
}

void SyncBridgeTest::ApplySyncChangesAndWait(
    std::unique_ptr<MetadataChangeList> metadata_change_list,
    EntityChangeList entity_changes) {
  base::Optional<ModelError> error =
      ApplySyncChanges(std::move(metadata_change_list),
                       std::move(entity_changes))
          .WaitForResult();
  EXPECT_FALSE(error) << error->ToString();
}

SyncBridgeTest::Future<std::unique_ptr<MetadataBatch>>
SyncBridgeTest::GetModelReadyToSyncFuture() {
  CallbackWaiter<std::unique_ptr<MetadataBatch>> waiter;
  auto cb = waiter.CallbackToWaitFor();
  // Forward calls to ModelReadyToSync() to the waiter object.
  ON_CALL(processor(), DoModelReadyToSync(_))
      .WillByDefault(Invoke([=](MetadataBatch* batch) {
        cb.Run(std::make_unique<MetadataBatch>(std::move(*batch)));
      }));
  return waiter.GetFuture();
}

// Test that prevents datatypes from accidentally returning the wrong value for
// SupportsGetStorageKey().
TEST_P(SyncBridgeTest, SupportsGetStorageKey) {
  InitializeBridgeAndWaitUntilReady();
  const auto entity1 = tester()->GetTestEntity1();
  if (bridge()->SupportsGetStorageKey()) {
    EXPECT_NE("", bridge()->GetStorageKey(
                      SpecificsToEntity(entity1.specifics).value()));
  } else {
    EXPECT_EQ("", bridge()->GetStorageKey(
                      SpecificsToEntity(entity1.specifics).value()));
  }
}

TEST_P(SyncBridgeTest, InitWithEmptyModel) {
  EXPECT_CALL(processor(), DoModelReadyToSync(Not(IsNull())));
  InitializeBridgeAndWaitUntilReady();
}

TEST_P(SyncBridgeTest, InitWithNonEmptyModel) {
  InitializeBridgeAndWaitUntilReady();
  tester()->CreateAndStoreLocalTestEntity1();

  EXPECT_CALL(processor(), DoModelReadyToSync(Not(IsNull())));
  RestartBridgeAndWaitUntilReady();
}

TEST_P(SyncBridgeTest, GetAllDataWithEmptyModel) {
  InitializeBridgeAndWaitUntilReady();
  // TODO/DONOTSUBMIT: This does not work for DeviceInfo, which populates one.
  EXPECT_THAT(GetAllDataFromBridge(), IsEmpty());
}

// Add two typed urls locally and verify bridge can get them from GetAllData.
TEST_P(SyncBridgeTest, GetAllData) {
  InitializeBridgeAndWaitUntilReady();
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  const auto entity2 = tester()->CreateAndStoreLocalTestEntity2();

  EXPECT_THAT(GetAllDataFromBridge(),
              UnorderedElementsAre(
                  Pair(entity1.storage_key, EqualsProto(entity1.specifics)),
                  Pair(entity2.storage_key, EqualsProto(entity2.specifics))));
}

TEST_P(SyncBridgeTest, GetAllDataAfterRestart) {
  InitializeBridgeAndWaitUntilReady();
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  const auto entity2 = tester()->CreateAndStoreLocalTestEntity2();

  RestartBridgeAndWaitUntilReady();

  EXPECT_THAT(GetAllDataFromBridge(),
              UnorderedElementsAre(
                  Pair(entity1.storage_key, EqualsProto(entity1.specifics)),
                  Pair(entity2.storage_key, EqualsProto(entity2.specifics))));
}

// Add two typed urls locally and verify bridge can get them from GetData.
TEST_P(SyncBridgeTest, GetData) {
  InitializeBridgeAndWaitUntilReady();

  // Add two urls to backend.
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  const auto entity2 = tester()->CreateAndStoreLocalTestEntity2();

  EXPECT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));
  EXPECT_THAT(tester()->GetData(entity2.storage_key),
              EqualsProto(entity2.specifics));
  EXPECT_THAT(tester()->GetData("unknown1"),
              EqualsProto(sync_pb::EntitySpecifics()));
}

TEST_P(SyncBridgeTest, DisableSync) {
  InitializeBridgeAndWaitUntilReady();
  StartSyncingAndWait({});

  EXPECT_CALL(processor(), DisableSync());
  bridge()->DisableSync();

  // In the current implementaton, it is assumed that a processor always exists.
  // This can be changed, but care must be taken.
  EXPECT_THAT(bridge()->change_processor(), Not(IsNull()));
}

// Starting sync with no local data should just push the synced url into the
// backend.
TEST_P(SyncBridgeTest, MergeWithoutLocal) {  // MergeUrlEmptyLocal) {
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  InitializeBridgeAndWaitUntilReady();

  const auto entity1 = tester()->GetTestEntity1();
  std::string storage_key;
  if (bridge()->SupportsGetStorageKey()) {
    storage_key =
        bridge()->GetStorageKey(SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(),
                UpdateStorageKey(HasSpecifics(entity1.specifics), _, _))
        .WillOnce(SaveArg<1>(&storage_key));
  }

  StartSyncingAndWait({entity1.specifics});

  // Check that the backend was updated correctly.
  EXPECT_THAT(GetAllDataFromBridge(),
              ElementsAre(Pair(storage_key, EqualsProto(entity1.specifics))));
}

// Starting sync with no sync data should just push the local url to sync.
TEST_P(SyncBridgeTest, MergeWithoutRemote) {  // MergeUrlEmptySync
  InitializeBridgeAndWaitUntilReady();
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();

  EXPECT_CALL(processor(), DoPut(entity1.storage_key,
                                 PointeeHasSpecifics(entity1.specifics), _));

  StartSyncingAndWait({});

  // Check that the local cache was is still correct.
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity1.storage_key, EqualsProto(entity1.specifics))));
}

// Add a typed url locally and one to sync with the same data. Starting sync
// should result in no changes.
TEST_P(SyncBridgeTest, MergeIdentical) {  // MergeUrlNoChange
  InitializeBridgeAndWaitUntilReady();
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();

  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  std::string storage_key;
  if (bridge()->SupportsGetStorageKey()) {
    storage_key =
        bridge()->GetStorageKey(SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(),
                UpdateStorageKey(HasSpecifics(entity1.specifics), _, _))
        .WillOnce(SaveArg<1>(&storage_key));
  }

  // Create the same data in sync.
  StartSyncingAndWait({entity1.specifics});

  // Check that the local cache was is still correct.
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity1.storage_key, EqualsProto(entity1.specifics))));
}

// Starting sync with both local and sync have same typed URL, but different
// visit. After merge, both local and sync should have two same visits.
TEST_P(SyncBridgeTest, SimpleMerge) {
  InitializeBridgeAndWaitUntilReady();
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();

  // Create conflicting data in sync.
  const auto mutated_specifics1 = tester()->GetMutatedTestEntity1Specifics();
  ASSERT_THAT(mutated_specifics1, Not(EqualsProto(entity1.specifics)));

  // Server wins by default, so no update expected.
  // TODO(mastiz): This should be overridable: use
  // GetConflictResolutionTests().
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);
  EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  StartSyncingAndWait({entity1.specifics});

  // Check that the local cache was is correct.
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity1.storage_key, EqualsProto(mutated_specifics1))));
}

// Create a local typed URL with one TYPED visit after sync has started. Check
// that sync is sent an ADD change for the new URL.
TEST_P(SyncBridgeTest, LocalAddAfterSyncStarted) {  // AddLocalTypedUrl
  InitializeBridgeAndWaitUntilReady();
  StartSyncingAndWait({});

  auto entity1 = tester()->GetTestEntity1();
  // We don't yet have information about the expectations, so we need to store
  // the arguments to have them verified later.
  std::string put_storage_key;
  base::RunLoop loop;
  EXPECT_CALL(processor(), DoPut(_, PointeeHasSpecifics(entity1.specifics), _))
      .WillRepeatedly(WithArg<0>(Invoke([&](const std::string& storage_key) {
        put_storage_key = storage_key;
        loop.Quit();
      })));
  // Create a local entity that is not already in sync. Wait until Put() is
  // received.
  const std::string storage_key =
      tester()->StoreEntityInLocalModel(entity1.local.get());
  loop.Run();
  EXPECT_THAT(put_storage_key, Eq(storage_key));
}

// Update a local typed URL that is already synced. Check that sync is sent an
// UPDATE for the existing url,
TEST_P(SyncBridgeTest, LocalUpdateForSyncedEntity) {  // UpdateLocalTypedUrl) {
  InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  StartSyncingAndWait({entity1.specifics});
  ASSERT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  // Mutating the local entity (below) should cause a call to Put().
  const auto mutated_specifics1 = tester()->GetMutatedTestEntity1Specifics();
  ASSERT_THAT(mutated_specifics1, Not(EqualsProto(entity1.specifics)));

  base::RunLoop loop;
  EXPECT_CALL(processor(), DoPut(entity1.storage_key,
                                 PointeeHasSpecifics(mutated_specifics1), _))
      .WillOnce(InvokeWithoutArgs(&loop, &base::RunLoop::Quit));
  tester()->MutateStoredTestEntity1();
  loop.Run();
  EXPECT_THAT(
      GetAllDataFromBridge(),
      ElementsAre(Pair(entity1.storage_key, EqualsProto(mutated_specifics1))));
}

// Locally delete an entity which is already synced. Check that the processor
// receives a call to Delete().
TEST_P(SyncBridgeTest, LocalDeletionSyncedEntity) {
  InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  StartSyncingAndWait({entity1.specifics});
  ASSERT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  EXPECT_CALL(processor(), Delete(entity1.storage_key, _));

  tester()->DeleteEntityFromLocalModel(entity1.storage_key);
  EXPECT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(sync_pb::EntitySpecifics()));

  // Verify that the entity is not resurrected after restart.
  RestartBridgeAndWaitUntilReady();
  EXPECT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(sync_pb::EntitySpecifics()));
}

// Create a remote typed URL and visit, then send to sync bridge after sync
// has started. Check that local DB is received the new URL and visit.
TEST_P(SyncBridgeTest, RemoteAddAfterSyncStarted) {  // AddUrlAndVisits) {
  InitializeBridgeAndWaitUntilReady();
  StartSyncingAndWait({});

  const auto entity1 = tester()->GetTestEntity1();

  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  std::string storage_key;
  if (bridge()->SupportsGetStorageKey()) {
    storage_key =
        bridge()->GetStorageKey(SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    // TODO(mastiz): How can we verify a storage_key here? The old tests
    // compared it to IntToStorageKey(1), but that seems overly strict.
    EXPECT_CALL(processor(),
                UpdateStorageKey(HasSpecifics(entity1.specifics), _, _))
        .WillOnce(SaveArg<1>(&storage_key));
  }

  ApplySyncChangesAndWait(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateAdd(storage_key,
                               SpecificsToEntity(entity1.specifics))});

  testing::Mock::VerifyAndClearExpectations(&processor());
  EXPECT_THAT(storage_key, Not(IsEmpty()));
  EXPECT_THAT(tester()->GetData(storage_key), EqualsProto(entity1.specifics));

  // Verify that the entity remains there after restart.
  RestartBridgeAndWaitUntilReady();
  EXPECT_THAT(tester()->GetData(storage_key), EqualsProto(entity1.specifics));
}

// Update a remote typed URL and create a new visit that is already synced, then
// send the update to sync bridge. Check that local DB is received an
// UPDATE for the existing url and new visit.
TEST_P(SyncBridgeTest, RemoteUpdateForSyncedEntity) {  // UpdateUrlAndVisits) {
  InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  StartSyncingAndWait({entity1.specifics});
  // Entity is synced as part of the test fixture.
  ASSERT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  const auto& mutated_specifics = tester()->GetMutatedTestEntity1Specifics();
  ASSERT_THAT(mutated_specifics, Not(EqualsProto(entity1.specifics)));

  // Remote update (below) should trigger no Put().
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);
  EXPECT_CALL(processor(), UpdateStorageKey(_, _, _)).Times(0);
  ApplySyncChangesAndWait(
      bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateUpdate(entity1.storage_key,
                                  SpecificsToEntity(mutated_specifics))});
  EXPECT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(mutated_specifics));
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
  const auto entity1 = tester()->CreateAndStoreLocalTestEntity1();
  StartSyncingAndWait({entity1.specifics});
  ASSERT_THAT(tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_CALL(processor(), Delete(_, _)).Times(0);
  EXPECT_CALL(processor(), DoPut(_, _, _)).Times(0);

  // Remote deletion.
  ApplySyncChangesAndWait(bridge()->CreateMetadataChangeList(),
                          {EntityChange::CreateDelete(entity1.storage_key)});

  EXPECT_THAT(tester()->GetData(entity1.storage_key),
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
  ApplySyncChangesAndWait(std::move(metadata_changes), {});

  std::unique_ptr<MetadataBatch> metadata_after_restart =
      RestartBridgeAndWaitUntilReady();
  ASSERT_NE(nullptr, metadata_after_restart);
  EXPECT_THAT(metadata_after_restart->GetModelTypeState(), EqualsProto(state));
}

}  // namespace syncer
