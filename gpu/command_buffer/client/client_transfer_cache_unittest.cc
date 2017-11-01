// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_transfer_cache.h"
#include "cc/paint/raw_memory_transfer_cache_entry.h"
#include "gpu/command_buffer/client/client_discardable_manager.h"
#include "gpu/command_buffer/client/client_discardable_texture_manager.h"
#include "gpu/command_buffer/client/gles2_interface_stub.cc"
#include "gpu/command_buffer/client/mapped_memory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {
namespace {
class FakeCommandBuffer : public CommandBuffer {
 public:
  FakeCommandBuffer() = default;
  ~FakeCommandBuffer() override {}
  // Overridden from CommandBuffer:
  State GetLastState() override {
    NOTREACHED();
    return State();
  };
  void Flush(int32_t put_offset) override { NOTREACHED(); }
  void OrderingBarrier(int32_t put_offset) override { NOTREACHED(); }
  State WaitForTokenInRange(int32_t start, int32_t end) override {
    NOTREACHED();

    return State();
  }
  State WaitForGetOffsetInRange(uint32_t set_get_buffer_count,
                                int32_t start,
                                int32_t end) override {
    NOTREACHED();
    return State();
  }
  void SetGetBuffer(int32_t transfer_buffer_id) override { NOTREACHED(); }
  scoped_refptr<gpu::Buffer> CreateTransferBuffer(size_t size,
                                                  int32_t* id) override {
    EXPECT_GE(size, 2048u);
    *id = next_id_++;
    active_ids_.insert(*id);
    std::unique_ptr<base::SharedMemory> shared_mem(new base::SharedMemory);
    shared_mem->CreateAndMapAnonymous(size);
    return MakeBufferFromSharedMemory(std::move(shared_mem), size);
  }
  void DestroyTransferBuffer(int32_t id) override {
    auto found = active_ids_.find(id);
    EXPECT_TRUE(found != active_ids_.end());
    active_ids_.erase(found);
  }

 private:
  int32_t next_id_ = 1;
  std::set<int32_t> active_ids_;
};

void UnlockClientHandleForTesting(
    const ClientDiscardableHandle& client_handle) {
  ServiceDiscardableHandle service_handle(client_handle.BufferForTesting(),
                                          client_handle.byte_offset(),
                                          client_handle.shm_id());
  service_handle.Unlock();
}

bool DeleteClientHandleForTesting(
    const ClientDiscardableHandle& client_handle) {
  ServiceDiscardableHandle service_handle(client_handle.BufferForTesting(),
                                          client_handle.byte_offset(),
                                          client_handle.shm_id());
  return service_handle.Delete();
}

}  // namespace

TEST(ClientTransferCacheTest, BasicUsage) {
  FakeCommandBuffer command_buffer;
  gles2::GLES2InterfaceStub gl;
  ClientTransferCache cache;

  std::vector<uint8_t> data;
  data.resize(100);
  std::unique_ptr<cc::ClientRawMemoryTransferCacheEntry> entry(
      new cc::ClientRawMemoryTransferCacheEntry(std::move(data)));
  cc::TransferCacheEntryId id =
      cache.CreateCacheEntry(&gl, &command_buffer, std::move(entry));
  EXPECT_FALSE(id.is_null());
  ClientDiscardableHandle handle =
      cache.DiscardableManagerForTesting()->GetHandle(id);
  EXPECT_TRUE(handle.IsLockedForTesting());

  cache.UnlockTransferCacheEntry(&gl, id);
  UnlockClientHandleForTesting(handle);
  EXPECT_FALSE(handle.IsLockedForTesting());

  EXPECT_TRUE(cache.LockTransferCacheEntry(id));
  EXPECT_TRUE(handle.IsLockedForTesting());
  UnlockClientHandleForTesting(handle);

  cache.DeleteTransferCacheEntry(&gl, id);
}

TEST(ClientTransferCacheTest, LockFail) {
  FakeCommandBuffer command_buffer;
  gles2::GLES2InterfaceStub gl;
  ClientTransferCache cache;

  std::vector<uint8_t> data;
  data.resize(100);
  std::unique_ptr<cc::ClientRawMemoryTransferCacheEntry> entry(
      new cc::ClientRawMemoryTransferCacheEntry(std::move(data)));
  cc::TransferCacheEntryId id =
      cache.CreateCacheEntry(&gl, &command_buffer, std::move(entry));
  EXPECT_FALSE(id.is_null());
  ClientDiscardableHandle handle =
      cache.DiscardableManagerForTesting()->GetHandle(id);
  EXPECT_TRUE(handle.IsLockedForTesting());

  cache.UnlockTransferCacheEntry(&gl, id);
  UnlockClientHandleForTesting(handle);
  EXPECT_FALSE(handle.IsLockedForTesting());

  DeleteClientHandleForTesting(handle);
  EXPECT_FALSE(cache.LockTransferCacheEntry(id));
  cache.DeleteTransferCacheEntry(&gl, id);
}

}  // namespace gpu
