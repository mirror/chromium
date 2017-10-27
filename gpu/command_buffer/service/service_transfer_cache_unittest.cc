// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/service_transfer_cache.h"

#include "gpu/command_buffer/client/client_test_helper.h"
#include "gpu/command_buffer/common/raw_memory_transfer_cache_entry.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder_mock.h"
#include "gpu/command_buffer/service/gpu_service_test.h"
#include "gpu/command_buffer/service/gpu_tracer.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/service/mailbox_manager_impl.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/mocks.h"
#include "gpu/command_buffer/service/service_discardable_manager.h"
#include "gpu/command_buffer/service/test_helper.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_image_stub.h"
#include "ui/gl/gl_mock.h"
#include "ui/gl/gl_switches.h"

using ::testing::Pointee;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::InSequence;

namespace gpu {
namespace gles2 {
namespace {

void CreateLockedHandlesForTesting(
    std::unique_ptr<ServiceDiscardableHandle>* service_handle,
    std::unique_ptr<ClientDiscardableHandle>* client_handle) {
  std::unique_ptr<base::SharedMemory> shared_mem(new base::SharedMemory);
  shared_mem->CreateAndMapAnonymous(sizeof(uint32_t));
  scoped_refptr<gpu::Buffer> buffer =
      MakeBufferFromSharedMemory(std::move(shared_mem), sizeof(uint32_t));

  client_handle->reset(new ClientDiscardableHandle(buffer, 0, 0));
  service_handle->reset(new ServiceDiscardableHandle(buffer, 0, 0));
}

ServiceDiscardableHandle CreateLockedServiceHandleForTesting() {
  std::unique_ptr<ServiceDiscardableHandle> service_handle;
  std::unique_ptr<ClientDiscardableHandle> client_handle;
  CreateLockedHandlesForTesting(&service_handle, &client_handle);
  return *service_handle;
}

}  // namespace

class ServiceTransferCacheTest : public GpuServiceTest {
 public:
  ServiceTransferCacheTest() {}
  ~ServiceTransferCacheTest() override {}

 protected:
  void SetUp() override {
    GpuServiceTest::SetUp();
    decoder_.reset(new MockGLES2Decoder(&command_buffer_service_, &outputter_));
    feature_info_ = new FeatureInfo();
    context_group_ = scoped_refptr<ContextGroup>(new ContextGroup(
        gpu_preferences_, false, &mailbox_manager_, nullptr, nullptr, nullptr,
        feature_info_, false, &image_manager_, nullptr, nullptr,
        GpuFeatureInfo(), &discardable_manager_, &transfer_cache_));
    TestHelper::SetupContextGroupInitExpectations(
        gl_.get(), DisallowedFeatures(), "", "", CONTEXT_TYPE_OPENGLES2, false);
    context_group_->Initialize(decoder_.get(), CONTEXT_TYPE_OPENGLES2,
                               DisallowedFeatures());
  }

  void TearDown() override {
    EXPECT_CALL(*gl_, DeleteTextures(TextureManager::kNumDefaultTextures, _));
    context_group_->Destroy(decoder_.get(), true);
    context_group_ = nullptr;
    EXPECT_EQ(0u, discardable_manager_.NumCacheEntriesForTesting());
    GpuServiceTest::TearDown();
  }

  MailboxManagerImpl mailbox_manager_;
  TraceOutputter outputter_;
  ImageManager image_manager_;
  ServiceDiscardableManager discardable_manager_;
  ServiceTransferCache transfer_cache_;
  GpuPreferences gpu_preferences_;
  scoped_refptr<FeatureInfo> feature_info_;
  FakeCommandBufferServiceBase command_buffer_service_;
  std::unique_ptr<MockGLES2Decoder> decoder_;
  scoped_refptr<gles2::ContextGroup> context_group_;
};

TEST_F(ServiceTransferCacheTest, BasicUsage) {
  uint64_t id = 1;
  size_t data_size = 100;
  TransferCacheEntryType type = TransferCacheEntryType::kRawMemory;
  auto handle = CreateLockedServiceHandleForTesting();
  std::unique_ptr<base::SharedMemory> shared_mem(new base::SharedMemory);
  shared_mem->CreateAndMapAnonymous(data_size);
  scoped_refptr<gpu::Buffer> buffer =
      MakeBufferFromSharedMemory(std::move(shared_mem), data_size);

  transfer_cache_.CreateLockedEntry(id, handle, type, std::move(buffer));
  auto* entry = transfer_cache_.GetEntry<RawMemoryTransferCacheEntry>(id);
  EXPECT_TRUE(entry);
  EXPECT_EQ(entry->Size(), data_size);

  transfer_cache_.UnlockEntry(id);
  EXPECT_FALSE(handle.IsLockedForTesting());

  transfer_cache_.DeleteEntry(id);
  EXPECT_TRUE(handle.IsDeletedForTesting());
  entry = transfer_cache_.GetEntry<RawMemoryTransferCacheEntry>(id);
  EXPECT_FALSE(entry);
}

}  // namespace gles2
}  // namespace gpu
