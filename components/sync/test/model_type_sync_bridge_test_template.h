// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_
#define COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_

#include <type_traits>

#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/mock_model_type_change_processor.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/model/sync_metadata_store.h"
#include "components/sync/protocol/proto_visitors.h"
#include "components/sync/protocol/sync.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_pb {

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

// std::future-like object that receives input from a callback.
template <typename T>
class CallbackWaiter {
 public:
  using value_type =
      typename std::remove_const<typename std::remove_reference<T>::type>::type;

  base::Callback<void(T)> CallbackToWaitFor() {
    CHECK(!state_);
    state_ = std::make_unique<State>();
    return base::Bind(&State::set, state_->AsWeakPtr());
  }

  base::Closure ClosureToWaitFor() {
    CHECK(!state_);
    state_ = std::make_unique<State>();
    return base::Bind(&State::set, state_->AsWeakPtr());
  }

  bool HasResult() const { return state_ && state_->result.has_value(); }

  value_type WaitForResult() {
    CHECK(state_);
    state_->loop.Run();
    CHECK(state_->result.has_value());
    value_type value = std::move(*state_->result);
    state_.reset();
    return value;
  }

 private:
  struct State : base::SupportsWeakPtr<State> {
    void set(T value) {
      result = std::move(value);
      loop.Quit();
    }

    base::RunLoop loop;
    base::Optional<value_type> result;
  };

  std::unique_ptr<State> state_;
};

// TODO / DONOTSUBMIT: Consider detemplatizing the tester (use EntityData more
// broadly and avoid NATIVE_T). If that were possible, we could migrate away
// from typed stuff.
template <typename N, typename S, typename Impl>
// TODO / DONOTSUBMIT: Some more ideas here.
// We need to test non-empty local data at startup (bridge creation / tester
// creation). Alternatives: 1. Feed in a vector of NATIVE_T during construction
// of TESTER. This could be done
//    via InitializeBridge(std::vector<NATIVE_T>).
// 2. Alternative above does not handle well test that use RestartBridge(), in
// particular
//    DisableSync. One solution would be to expose
//    RestartBridge(std::unique_ptr<TESTER>* tester);
// NOTE: See DeviceInfo TestWithLocalMetadata and TestWithLocalDataAndMetadata.
class ModelTypeSyncBridgeTester {
 public:
  // NATIVE_T must hold sufficient information to create local entities.
  using NATIVE_T = N;
  // SPECIFICS is the sync proto, holding analogous information to NATIVE_T.
  using SPECIFICS = S;

  virtual ~ModelTypeSyncBridgeTester() = default;

  static EntityDataPtr SpecificsToEntity(const SPECIFICS& specifics) {
    return Impl::SpecificsToEntityImpl(specifics);
  }

  static const SPECIFICS& SelectSpecifics(
      const sync_pb::EntitySpecifics& entity_specifics) {
    return Impl::SelectSpecificsImpl(entity_specifics);
  }

  static testing::Matcher<const EntityData&> HasSpecifics(
      const SPECIFICS& expected) {
    return testing::Field(
        &EntityData::specifics,
        testing::ResultOf(&SelectSpecifics, EqualsProto(expected)));
  }

  static testing::Matcher<EntityData*> PointeeHasSpecifics(
      const SPECIFICS& expected) {
    return testing::Pointee(HasSpecifics(expected));
  }

  struct TestEntityData {
    NATIVE_T native;
    SPECIFICS specifics;
  };

  struct TestEntityDataWithStorageKey {
    std::string storage_key;
    NATIVE_T native;
    SPECIFICS specifics;
  };

  virtual ModelTypeSyncBridge* bridge() = 0;
  // TODO(mastiz) / DONOTSUBMIT: Consider making the two below static.
  virtual TestEntityData GetTestEntity1() const = 0;
  virtual TestEntityData GetTestEntity2() const = 0;
  // Creation of entities that are stored locally. On return, the entity
  // is present in the local/native model, and the return value represents
  // the storage key.
  virtual std::string StoreEntityInLocalModel(NATIVE_T* native) = 0;
  virtual void DeleteEntityFromLocalModel(const std::string& storage_key) = 0;
  // Mutations.
  virtual SPECIFICS GetMutatedTestEntity1Specifics() const = 0;
  virtual void MutateStoredTestEntity1() = 0;

  // Convenience helpers.
  TestEntityDataWithStorageKey CreateAndStoreNativeTestEntity1() {
    TestEntityData entity = GetTestEntity1();
    std::string storage_key = StoreEntityInLocalModel(&entity.native);
    return {storage_key, std::move(entity.native), std::move(entity.specifics)};
  }

  TestEntityDataWithStorageKey CreateAndStoreNativeTestEntity2() {
    TestEntityData entity = GetTestEntity2();
    std::string storage_key = StoreEntityInLocalModel(&entity.native);
    return {storage_key, std::move(entity.native), std::move(entity.specifics)};
  }

  // Starts sync for the bridge with |initial_data| as the initial sync data.
  CallbackWaiter<const base::Optional<ModelError>&> StartSyncing(
      const std::vector<SPECIFICS>& initial_data) WARN_UNUSED_RESULT {
    EntityChangeList entity_change_list;
    for (const auto& specifics : initial_data) {
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
    return waiter;
  }

  // Same above but waits until the initial merge is finished.
  void StartSyncingAndWait(const std::vector<SPECIFICS>& initial_data) {
    base::Optional<ModelError> error =
        StartSyncing(initial_data).WaitForResult();
    EXPECT_FALSE(error) << error->ToString();
  }

  void ApplySyncChangesAndWait(
      std::unique_ptr<MetadataChangeList> metadata_change_list,
      EntityChangeList entity_changes) {
    CallbackWaiter<const base::Optional<ModelError>&> waiter;
    bridge()->ApplySyncChanges2(std::move(metadata_change_list),
                                std::move(entity_changes),
                                waiter.CallbackToWaitFor());
    base::Optional<ModelError> error = waiter.WaitForResult();
    EXPECT_FALSE(error) << error->ToString();
  }

  std::map<std::string, SPECIFICS> GetAllData() {
    CallbackWaiter<std::unique_ptr<DataBatch>> waiter;
    bridge()->GetAllData(waiter.CallbackToWaitFor());

    std::unique_ptr<DataBatch> batch = waiter.WaitForResult();
    EXPECT_NE(nullptr, batch);

    std::map<std::string, SPECIFICS> storage_key_to_specifics;
    if (batch != nullptr) {
      while (batch->HasNext()) {
        const KeyAndData& pair = batch->Next();
        storage_key_to_specifics[pair.first] =
            SelectSpecifics(pair.second->specifics);
      }
    }
    return storage_key_to_specifics;
  }

  SPECIFICS GetData(const std::string& storage_key) {
    CallbackWaiter<std::unique_ptr<DataBatch>> waiter;
    bridge()->GetData({storage_key}, waiter.CallbackToWaitFor());

    std::unique_ptr<DataBatch> batch = waiter.WaitForResult();
    EXPECT_NE(nullptr, batch);

    SPECIFICS specifics;
    if (batch != nullptr && batch->HasNext()) {
      const KeyAndData& pair = batch->Next();
      specifics = SelectSpecifics(pair.second->specifics);
      EXPECT_FALSE(batch->HasNext());
    }
    return specifics;
  }
};

template <typename TESTER>
class SyncBridgeTest : public testing::Test {
 public:
  using SPECIFICS = typename TESTER::SPECIFICS;
  using NATIVE_T = typename TESTER::NATIVE_T;

  SyncBridgeTest() {
    using testing::_;
    // Even if we use NiceMock, let's be strict about errors and let tests
    // explicitly list them.
    EXPECT_CALL(processor_, ReportError(_, _)).Times(0);

    ON_CALL(processor_, IsTrackingMetadata())
        .WillByDefault(testing::Return(true));
  }

  ~SyncBridgeTest() override {}

  TESTER* tester() {
    CHECK(tester_) << "Bridge not initialized";
    return tester_.get();
  }

  ModelTypeSyncBridge* bridge() {
    ModelTypeSyncBridge* bridge = tester()->bridge();
    CHECK(bridge);
    return bridge;
  }

  MockModelTypeChangeProcessor* processor() { return &processor_; };

  CallbackWaiter<std::unique_ptr<MetadataBatch>> InitializeBridge()
      WARN_UNUSED_RESULT {
    auto waiter = GetModelReadyToSyncWaiter();
    // Initialize the tester (which includes the bridge).
    tester_ = std::make_unique<TESTER>(
        MockModelTypeChangeProcessor::FactoryForBridgeTest(&processor_));
    return waiter;
  }

  void InitializeBridgeAndWaitUntilReady() {
    InitializeBridge().WaitForResult();
  }

  void RestartBridgeAndWaitUntilReady() {
    CHECK(tester_);
    auto waiter = GetModelReadyToSyncWaiter();
    TESTER::Restart(&tester_);
    waiter.WaitForResult();
  }

  std::map<std::string, SPECIFICS> GetAllDataFromBridge() {
    return tester()->GetAllData();
  }

 private:
  CallbackWaiter<std::unique_ptr<MetadataBatch>> GetModelReadyToSyncWaiter() {
    using testing::_;
    CallbackWaiter<std::unique_ptr<MetadataBatch>> waiter;
    auto cb = waiter.CallbackToWaitFor();
    // Forward calls to ModelReadyToSync() to the waiter object.
    ON_CALL(*processor(), DoModelReadyToSync(_))
        .WillByDefault(testing::Invoke([=](MetadataBatch* batch) {
          cb.Run(std::make_unique<MetadataBatch>(std::move(*batch)));
        }));
    return waiter;
  }

  base::MessageLoop message_loop_;
  testing::NiceMock<MockModelTypeChangeProcessor> processor_;
  std::unique_ptr<TESTER> tester_;
};

TYPED_TEST_CASE_P(SyncBridgeTest);

TYPED_TEST_P(SyncBridgeTest, InitWithEmptyModel) {
  using testing::_;

  EXPECT_CALL(*this->processor(),
              DoModelReadyToSync(testing::Not(testing::IsNull())));
  this->InitializeBridgeAndWaitUntilReady();
}

TYPED_TEST_P(SyncBridgeTest, InitWithNonEmptyModel) {
  // TODO: Reimplement using reset bridge.
  /*
  using testing::_;

  base::RunLoop loop;
  EXPECT_CALL(*this->processor(),
              DoModelReadyToSync(testing::Not(testing::IsNull())))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));

  this->tester()->CreateAndStoreNativeTestEntity1();
  this->tester()->StartSyncingAndWait({});
  loop.Run();*/
}

TYPED_TEST_P(SyncBridgeTest, GetAllDataWithEmptyModel) {
  this->InitializeBridgeAndWaitUntilReady();
  // TODO/DONOTSUBMIT: This does not work for DeviceInfo, which populates one.
  // EXPECT_THAT(this->GetAllDataFromBridge(), testing::IsEmpty());
}

// Add two typed urls locally and verify bridge can get them from GetAllData.
TYPED_TEST_P(SyncBridgeTest, GetAllData) {
  this->InitializeBridgeAndWaitUntilReady();

  // Add two urls to backend.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  const auto entity2 = this->tester()->CreateAndStoreNativeTestEntity2();

  EXPECT_THAT(
      this->GetAllDataFromBridge(),
      testing::UnorderedElementsAre(
          testing::Pair(entity1.storage_key, EqualsProto(entity1.specifics)),
          testing::Pair(entity2.storage_key, EqualsProto(entity2.specifics))));
}

// Add two typed urls locally and verify bridge can get them from GetData.
TYPED_TEST_P(SyncBridgeTest, GetData) {
  this->InitializeBridgeAndWaitUntilReady();

  // Add two urls to backend.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  const auto entity2 = this->tester()->CreateAndStoreNativeTestEntity2();

  EXPECT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));
  EXPECT_THAT(this->tester()->GetData(entity2.storage_key),
              EqualsProto(entity2.specifics));
  EXPECT_THAT(this->tester()->GetData("unknown1"),
              EqualsProto(typename TypeParam::SPECIFICS()));
}

TYPED_TEST_P(SyncBridgeTest, DisableSync) {
  this->InitializeBridgeAndWaitUntilReady();
  this->tester()->StartSyncingAndWait({});

  EXPECT_CALL(*this->processor(), DisableSync());
  this->bridge()->DisableSync();

  // In the current implementaton, it is assumed that a processor always exists.
  // This can be changed, but care must be taken.
  EXPECT_THAT(this->bridge()->change_processor(),
              testing::Not(testing::IsNull()));
}

// Starting sync with no local data should just push the synced url into the
// backend.
TYPED_TEST_P(SyncBridgeTest, MergeWithoutLocal) {  // MergeUrlEmptyLocal) {
  using testing::_;

  EXPECT_CALL(*this->processor(), DoPut(_, _, _)).Times(0);

  this->InitializeBridgeAndWaitUntilReady();

  const auto entity1 = this->tester()->GetTestEntity1();
  std::string storage_key;
  if (this->bridge()->SupportsGetStorageKey()) {
    storage_key = this->bridge()->GetStorageKey(
        TypeParam::SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(*this->processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(
        *this->processor(),
        UpdateStorageKey(TypeParam::HasSpecifics(entity1.specifics), _, _))
        .WillOnce(testing::SaveArg<1>(&storage_key));
  }

  this->tester()->StartSyncingAndWait({entity1.specifics});

  // Check that the backend was updated correctly.
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(
                  testing::Pair(storage_key, EqualsProto(entity1.specifics))));
}

// Starting sync with no sync data should just push the local url to sync.
TYPED_TEST_P(SyncBridgeTest, MergeWithoutRemote) {  // MergeUrlEmptySync
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();

  EXPECT_CALL(*this->processor(),
              DoPut(entity1.storage_key,
                    TypeParam::PointeeHasSpecifics(entity1.specifics), _));

  this->tester()->StartSyncingAndWait({});

  // Check that the local cache was is still correct.
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key, EqualsProto(entity1.specifics))));
}

// Add a typed url locally and one to sync with the same data. Starting sync
// should result in no changes.
TYPED_TEST_P(SyncBridgeTest, MergeIdentical) {  // MergeUrlNoChange
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();

  EXPECT_CALL(*this->processor(), DoPut(_, _, _)).Times(0);

  std::string storage_key;
  if (this->bridge()->SupportsGetStorageKey()) {
    storage_key = this->bridge()->GetStorageKey(
        TypeParam::SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(*this->processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(
        *this->processor(),
        UpdateStorageKey(TypeParam::HasSpecifics(entity1.specifics), _, _))
        .WillOnce(testing::SaveArg<1>(&storage_key));
  }

  // Create the same data in sync.
  this->tester()->StartSyncingAndWait({entity1.specifics});

  // Check that the local cache was is still correct.
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key, EqualsProto(entity1.specifics))));
}

// Starting sync with both local and sync have same typed URL, but different
// visit. After merge, both local and sync should have two same visits.
TYPED_TEST_P(SyncBridgeTest, MergeConflict) {  // SimpleMerge
  // TODO / DONOTSUBIT: This one seems hard to generalize.
}

// Create a local typed URL with one TYPED visit after sync has started. Check
// that sync is sent an ADD change for the new URL.
TYPED_TEST_P(SyncBridgeTest, LocalAddAfterSyncStarted) {  // AddLocalTypedUrl
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();
  this->tester()->StartSyncingAndWait({});

  auto entity1 = this->tester()->GetTestEntity1();
  // We don't yet have information about the expectations, so we need to store
  // the arguments to have them verified later.
  std::string put_storage_key;
  base::RunLoop loop;
  EXPECT_CALL(*this->processor(),
              DoPut(_, TypeParam::PointeeHasSpecifics(entity1.specifics), _))
      .WillRepeatedly(testing::WithArg<0>(
          testing::Invoke([&](const std::string& storage_key) {
            put_storage_key = storage_key;
            loop.Quit();
          })));
  // Create a local entity that is not already in sync. Wait until Put() is
  // received.
  const std::string storage_key =
      this->tester()->StoreEntityInLocalModel(&entity1.native);
  loop.Run();
  EXPECT_THAT(put_storage_key, testing::Eq(storage_key));
}

// Update a local typed URL that is already synced. Check that sync is sent an
// UPDATE for the existing url,
TYPED_TEST_P(SyncBridgeTest,
             LocalUpdateForSyncedEntity) {  // UpdateLocalTypedUrl) {
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  this->tester()->StartSyncingAndWait({entity1.specifics});
  ASSERT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  // Mutating the local entity (below) should cause a call to Put().
  const auto mutated_specifics1 =
      this->tester()->GetMutatedTestEntity1Specifics();
  ASSERT_THAT(mutated_specifics1, testing::Not(EqualsProto(entity1.specifics)));

  base::RunLoop loop;
  EXPECT_CALL(*this->processor(),
              DoPut(entity1.storage_key,
                    TypeParam::PointeeHasSpecifics(mutated_specifics1), _))
      .WillOnce(testing::InvokeWithoutArgs(&loop, &base::RunLoop::Quit));
  this->tester()->MutateStoredTestEntity1();
  loop.Run();
  EXPECT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key, EqualsProto(mutated_specifics1))));
}

// Locally delete an entity which is already synced. Check that the processor
// receives a call to Delete().
TYPED_TEST_P(SyncBridgeTest, LocalDeletionSyncedEntity) {
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  this->tester()->StartSyncingAndWait({entity1.specifics});
  ASSERT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_CALL(*this->processor(), DoPut(_, _, _)).Times(0);

  EXPECT_CALL(*this->processor(), Delete(entity1.storage_key, _));

  this->tester()->DeleteEntityFromLocalModel(entity1.storage_key);
  EXPECT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(typename TypeParam::SPECIFICS()));

  // Verify that the entity is not resurrected after restart.
  this->RestartBridgeAndWaitUntilReady();
  EXPECT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(typename TypeParam::SPECIFICS()));
}

// Create a remote typed URL and visit, then send to sync bridge after sync
// has started. Check that local DB is received the new URL and visit.
TYPED_TEST_P(SyncBridgeTest, RemoteAddAfterSyncStarted) {  // AddUrlAndVisits) {
  using testing::_;

  this->InitializeBridgeAndWaitUntilReady();
  this->tester()->StartSyncingAndWait({});

  const auto entity1 = this->tester()->GetTestEntity1();

  EXPECT_CALL(*this->processor(), DoPut(_, _, _)).Times(0);

  std::string storage_key;
  if (this->bridge()->SupportsGetStorageKey()) {
    storage_key = this->bridge()->GetStorageKey(
        TypeParam::SpecificsToEntity(entity1.specifics).value());
    // For bridges that do support SupportsGetStorageKey(), no call to
    // UpdateStorageKey() is expected.
    EXPECT_CALL(*this->processor(), UpdateStorageKey(_, _, _)).Times(0);
  } else {
    // For bridges that do not support SupportsGetStorageKey(), a call to
    // UpdateStorageKey() is expected.
    // TODO(mastiz): How can we verify a storage_key here? The old tests
    // compared it to IntToStorageKey(1), but that seems overly strict.
    EXPECT_CALL(
        *this->processor(),
        UpdateStorageKey(TypeParam::HasSpecifics(entity1.specifics), _, _))
        .WillOnce(testing::SaveArg<1>(&storage_key));
  }

  this->tester()->ApplySyncChangesAndWait(
      this->bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateAdd(
          storage_key, TypeParam::SpecificsToEntity(entity1.specifics))});

  testing::Mock::VerifyAndClearExpectations(this->processor());
  EXPECT_THAT(storage_key, testing::Not(testing::IsEmpty()));
  EXPECT_THAT(this->tester()->GetData(storage_key),
              EqualsProto(entity1.specifics));
}

// Update a remote typed URL and create a new visit that is already synced, then
// send the update to sync bridge. Check that local DB is received an
// UPDATE for the existing url and new visit.
TYPED_TEST_P(SyncBridgeTest,
             RemoteUpdateForSyncedEntity) {  // UpdateUrlAndVisits) {
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  this->tester()->StartSyncingAndWait({entity1.specifics});
  // Entity is synced as part of the test fixture.
  // ASSERT_THAT(this->tester()->GetData(entity1.storage_key),
  //            EqualsProto(entity1.specifics));
  ASSERT_THAT(this->GetAllDataFromBridge(),
              testing::ElementsAre(testing::Pair(
                  entity1.storage_key, EqualsProto(entity1.specifics))));

  const auto& mutated_specifics =
      this->tester()->GetMutatedTestEntity1Specifics();
  ASSERT_THAT(mutated_specifics, testing::Not(EqualsProto(entity1.specifics)));

  // Remote update (below) should trigger no Put().
  EXPECT_CALL(*this->processor(), DoPut(_, _, _)).Times(0);
  EXPECT_CALL(*this->processor(), UpdateStorageKey(_, _, _)).Times(0);
  this->tester()->ApplySyncChangesAndWait(
      this->bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateUpdate(
          entity1.storage_key,
          TypeParam::SpecificsToEntity(mutated_specifics))});
  EXPECT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(mutated_specifics));
}

// Delete a typed urls which already synced. Check that local DB receives the
// DELETE changes.
TYPED_TEST_P(SyncBridgeTest,
             RemoteDeletionForSyncedEntity) {  // DeleteUrlAndVisits) {
  using testing::_;
  this->InitializeBridgeAndWaitUntilReady();

  // Create the same data locally and in sync.
  const auto entity1 = this->tester()->CreateAndStoreNativeTestEntity1();
  this->tester()->StartSyncingAndWait({entity1.specifics});
  ASSERT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(entity1.specifics));

  // Check TypedUrlSyncBridge did not receive update since the update is
  // trigered by it.
  EXPECT_CALL(*this->processor(), Delete(_, _)).Times(0);
  EXPECT_CALL(*this->processor(), DoPut(_, _, _)).Times(0);

  // Remote deletion.
  this->tester()->ApplySyncChangesAndWait(
      this->bridge()->CreateMetadataChangeList(),
      {EntityChange::CreateDelete(entity1.storage_key)});

  EXPECT_THAT(this->tester()->GetData(entity1.storage_key),
              EqualsProto(typename TypeParam::SPECIFICS()));
}

TYPED_TEST_P(SyncBridgeTest, EmptyDataReconciliation) {
  /*
    this->tester()->InitializeBridgeAndWaitUntilReady();
  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
  */
}

TYPED_TEST_P(SyncBridgeTest, EmptyDataReconciliationSlowLoad) {
  /*
  this->tester()->InitializeBridge();
  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
  this->tester()->WaitUntilBridgeReady();
  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
  */
}

TYPED_TEST_P(SyncBridgeTest, LocalProviderSubscription) {
  /*
  InitializeAndPump();

  EXPECT_EQ(0u, bridge()->GetAllDeviceInfo().size());
  local_device()->Initialize(CreateModel(1));
  base::RunLoop().RunUntilIdle();

  const DeviceInfoList devices = bridge()->GetAllDeviceInfo();
  ASSERT_EQ(1u, devices.size());
  EXPECT_TRUE(local_device()->GetLocalDeviceInfo()->Equals(*devices[0]));
  */
}

REGISTER_TYPED_TEST_CASE_P(SyncBridgeTest,
                           InitWithEmptyModel,
                           InitWithNonEmptyModel,
                           GetAllDataWithEmptyModel,
                           GetAllData,
                           GetData,
                           DisableSync,
                           MergeWithoutLocal,
                           MergeWithoutRemote,
                           MergeIdentical,
                           MergeConflict,
                           LocalAddAfterSyncStarted,
                           LocalUpdateForSyncedEntity,
                           LocalDeletionSyncedEntity,
                           RemoteAddAfterSyncStarted,
                           RemoteUpdateForSyncedEntity,
                           RemoteDeletionForSyncedEntity,
                           EmptyDataReconciliation,
                           EmptyDataReconciliationSlowLoad,
                           LocalProviderSubscription);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_
