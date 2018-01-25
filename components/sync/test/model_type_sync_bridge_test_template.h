// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_
#define COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/mock_model_type_change_processor.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/model/sync_metadata_store.h"
#include "components/sync/protocol/sync.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class ModelTypeSyncBridgeTester {
 public:
  using Factory =
      base::RepeatingCallback<std::unique_ptr<ModelTypeSyncBridgeTester>(
          ModelTypeSyncBridge::ChangeProcessorFactory)>;

  template <typename T>
  static Factory GetFactory() {
    return base::BindRepeating(
        [](syncer::ModelTypeSyncBridge::ChangeProcessorFactory
               change_processor_factory)
            -> std::unique_ptr<ModelTypeSyncBridgeTester> {
          return std::make_unique<T>(change_processor_factory);
        });
  }

  ModelTypeSyncBridgeTester();
  virtual ~ModelTypeSyncBridgeTester();

  struct LocalType {
    virtual ~LocalType() = 0;
  };

  struct TestEntityData {
    TestEntityData(std::unique_ptr<LocalType> local,
                   const sync_pb::EntitySpecifics& specifics);
    TestEntityData(TestEntityData&&);
    ~TestEntityData();

    std::unique_ptr<LocalType> local;
    sync_pb::EntitySpecifics specifics;
  };

  struct TestEntityDataWithStorageKey {
    TestEntityDataWithStorageKey(TestEntityData data,
                                 const std::string& storage_key);
    TestEntityDataWithStorageKey(TestEntityDataWithStorageKey&&);
    ~TestEntityDataWithStorageKey();

    std::string storage_key;
    std::unique_ptr<LocalType> local;
    sync_pb::EntitySpecifics specifics;
  };

  // Restarts should be effectively as shutting down everything and restarting,
  // simulating a persitency layer that is carried over.
  virtual void Restart() = 0;
  virtual ModelTypeSyncBridge* bridge() = 0;
  // Let the tester provide a bunch of test entities. Requirements are:
  // - The returned list must be non-empty.
  // - The first entity must be locally modifiable.
  virtual std::vector<TestEntityData> GetTestEntities() = 0;
  // Creation of entities that are stored locally. On return, the entity
  // is present in the local/local model, and the return value represents
  // the storage key.
  virtual std::string StoreEntityInLocalModel(LocalType* local) = 0;
  virtual void DeleteEntityFromLocalModel(const std::string& storage_key) = 0;
  // Mutations.
  virtual void MutateStoredTestEntity(const std::string& storage_key,
                                      LocalType* local) = 0;
  virtual void MutateTestEntitySpecifics(
      sync_pb::EntitySpecifics* specifics) = 0;
  // Some data types (DeviceInfo) have no actual local API for mutations, and
  // instead the set of locally created entities is fixed. This represents a map
  // from storage key to specifics.
  virtual std::map<std::string, sync_pb::EntitySpecifics>
  GetFixedLocalEntities();

  struct ConflictResolution {
    ConflictResolution(std::unique_ptr<LocalType> local,
                       const sync_pb::EntitySpecifics& remote_specifics,
                       const sync_pb::EntitySpecifics& expected_result);
    ConflictResolution(ConflictResolution&&);
    ~ConflictResolution();

    std::unique_ptr<LocalType> local;
    sync_pb::EntitySpecifics remote_specifics;
    sync_pb::EntitySpecifics expected_result;
  };

  // Conflict resolution. By default, server wins.
  virtual std::vector<ConflictResolution> GetConflictResolutionTests();

  // Convenience helpers.
  TestEntityDataWithStorageKey CreateAndStoreLocalTestEntity();
  std::map<std::string, sync_pb::EntitySpecifics> GetAllData();
  sync_pb::EntitySpecifics GetData(const std::string& storage_key);
  bool CanModifyLocalModel();
};

class SyncBridgeTest
    : public ::testing::TestWithParam<ModelTypeSyncBridgeTester::Factory> {
 public:
  SyncBridgeTest();
  ~SyncBridgeTest() override;

  ModelTypeSyncBridgeTester* tester();
  ModelTypeSyncBridge* bridge();
  MockModelTypeChangeProcessor& processor();

  std::unique_ptr<MetadataBatch> InitializeBridgeAndWaitUntilReady();
  std::unique_ptr<MetadataBatch> RestartBridgeAndWaitUntilReady();
  void StartSyncing(const std::vector<sync_pb::EntitySpecifics> initial_data);
  void ApplySyncChangesAndWait(
      std::unique_ptr<MetadataChangeList> metadata_change_list,
      EntityChangeList entity_changes);
  std::map<std::string, sync_pb::EntitySpecifics> GetAllDataFromBridge();

 private:
  base::MessageLoop message_loop_;
  testing::NiceMock<MockModelTypeChangeProcessor> processor_;
  std::unique_ptr<ModelTypeSyncBridgeTester> tester_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_MODEL_TYPE_SYNC_BRIDGE_TEST_TEMPLATE_H_
