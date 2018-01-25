// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/model_type_sync_bridge.h"

#include <utility>

#include "base/bind.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/mock_model_type_change_processor.h"
#include "components/sync/model/stub_model_type_sync_bridge.h"
#include "components/sync/protocol/model_type_state.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

class MockModelTypeSyncBridge : public StubModelTypeSyncBridge {
 public:
  MockModelTypeSyncBridge()
      : StubModelTypeSyncBridge(
            base::Bind(&MockModelTypeSyncBridge::CreateProcessor,
                       base::Unretained(this))) {}
  ~MockModelTypeSyncBridge() override {}

  MockModelTypeChangeProcessor* change_processor() const {
    return static_cast<MockModelTypeChangeProcessor*>(
        ModelTypeSyncBridge::change_processor());
  }

  bool processor_disable_sync_called() const {
    return processor_disable_sync_called_;
  }

 private:
  std::unique_ptr<ModelTypeChangeProcessor> CreateProcessor(
      ModelType type,
      ModelTypeSyncBridge* bridge) {
    return std::make_unique<MockModelTypeChangeProcessor>();
  }

  void OnProcessorDisableSync() { processor_disable_sync_called_ = true; }

  bool processor_disable_sync_called_ = false;
};

class ModelTypeSyncBridgeTest : public ::testing::Test {
 public:
  ModelTypeSyncBridgeTest() {}
  ~ModelTypeSyncBridgeTest() override {}

  void OnSyncStarting() {
    bridge_.OnSyncStarting(
        ModelErrorHandler(),
        base::Bind(&ModelTypeSyncBridgeTest::OnProcessorStarted,
                   base::Unretained(this)));
  }

  bool start_callback_called() const { return start_callback_called_; }
  MockModelTypeSyncBridge* bridge() { return &bridge_; }

 private:
  void OnProcessorStarted(
      std::unique_ptr<ActivationContext> activation_context) {
    start_callback_called_ = true;
  }

  bool start_callback_called_ = false;
  MockModelTypeSyncBridge bridge_;
};

// OnSyncStarting should create a processor and call OnSyncStarting on it.
TEST_F(ModelTypeSyncBridgeTest, OnSyncStarting) {
  EXPECT_FALSE(start_callback_called());
  OnSyncStarting();

  // FakeModelTypeProcessor is the one that calls the callback, so if it was
  // called then we know the call on the processor was made.
  EXPECT_TRUE(start_callback_called());
}

// ResolveConflicts should return USE_REMOTE unless the remote data is deleted.
TEST_F(ModelTypeSyncBridgeTest, DefaultConflictResolution) {
  EntityData local_data;
  EntityData remote_data;

  // There is no deleted/deleted case because that's not a conflict.

  local_data.specifics.mutable_preference()->set_value("value");
  EXPECT_FALSE(local_data.is_deleted());
  EXPECT_TRUE(remote_data.is_deleted());
  EXPECT_EQ(ConflictResolution::USE_LOCAL,
            bridge()->ResolveConflict(local_data, remote_data).type());

  remote_data.specifics.mutable_preference()->set_value("value");
  EXPECT_FALSE(local_data.is_deleted());
  EXPECT_FALSE(remote_data.is_deleted());
  EXPECT_EQ(ConflictResolution::USE_REMOTE,
            bridge()->ResolveConflict(local_data, remote_data).type());

  local_data.specifics.clear_preference();
  EXPECT_TRUE(local_data.is_deleted());
  EXPECT_FALSE(remote_data.is_deleted());
  EXPECT_EQ(ConflictResolution::USE_REMOTE,
            bridge()->ResolveConflict(local_data, remote_data).type());
}

}  // namespace syncer
