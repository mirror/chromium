// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_CLIENT_DISCARDABLE_MANAGER_H_
#define GPU_COMMAND_BUFFER_CLIENT_CLIENT_DISCARDABLE_MANAGER_H_

#include <map>
#include <set>

#include "base/containers/queue.h"
#include "gpu/command_buffer/client/gles2_cmd_helper.h"
#include "gpu/command_buffer/common/command_buffer.h"
#include "gpu/command_buffer/common/discardable_handle.h"
#include "gpu/gpu_export.h"

namespace gpu {

// ClientDiscardableManager is a helper class used by the client GLES2
// implementation.
class GPU_EXPORT ClientDiscardableManager {
 public:
  ClientDiscardableManager();
  ~ClientDiscardableManager();
  DiscardableHandleId CreateHandle(gles2::GLES2CmdHelper* command_buffer);
  bool LockHandle(DiscardableHandleId handle_id);
  void FreeHandle(DiscardableHandleId handle_id);
  bool HandleIsValid(DiscardableHandleId handle_id) const;

  // Test only functions.
  void CheckPendingForTesting(CommandBuffer* command_buffer) {
    CheckPending(command_buffer);
  }
  void SetElementCountForTesting(uint32_t count) {
    elements_per_allocation_ = count;
    allocation_size_ = count * element_size_;
  }
  ClientDiscardableHandle GetHandleForTesting(DiscardableHandleId handle_id);

 private:
  void FindAllocation(CommandBuffer* command_buffer,
                      scoped_refptr<Buffer>* buffer,
                      int32_t* shm_id,
                      uint32_t* offset);
  void ReturnAllocation(CommandBuffer* command_buffer,
                        const ClientDiscardableHandle& handle);
  void CheckPending(CommandBuffer* command_buffer);

 private:
  size_t allocation_size_;
  size_t element_size_ = sizeof(base::subtle::Atomic32);
  uint32_t elements_per_allocation_ =
      static_cast<uint32_t>(allocation_size_ / element_size_);

  struct Allocation;
  std::vector<std::unique_ptr<Allocation>> allocations_;
  std::map<DiscardableHandleId, ClientDiscardableHandle> handles_;

  // Handles that are pending service deletion, and can be re-used once
  // ClientDiscardableHandle::CanBeReUsed returns true.
  base::queue<ClientDiscardableHandle> pending_handles_;

  DISALLOW_COPY_AND_ASSIGN(ClientDiscardableManager);
};

// A helper class used to manage discardable textures. Makes use of
// ClientDiscardableManager. When the GLES2 impl is done with a texture (the
// texture is being deleted), it should call FreeTexture to allow helper memory
// to be reclaimed.
class GPU_EXPORT ClientDiscardableTextureManager {
 public:
  ClientDiscardableTextureManager(ClientDiscardableManager* discardable_manager);
  ~ClientDiscardableTextureManager();
  DiscardableHandleId InitializeTexture(gles2::GLES2CmdHelper* command_buffer,
                                        uint32_t texture_id);
  bool LockTexture(uint32_t texture_id);
  void FreeTexture(uint32_t texture_id);
  bool TextureIsValid(uint32_t texture_id) const;

  // Test only functions.
  void CheckPendingForTesting(CommandBuffer* command_buffer) {
    discardable_manager_->CheckPendingForTesting(command_buffer);
  }
  void SetElementCountForTesting(uint32_t count) {
    discardable_manager_->SetElementCountForTesting(count);
  }
  ClientDiscardableHandle GetHandleForTesting(uint32_t texture_id);

 private:
  std::map<uint32_t, DiscardableHandleId> texture_id_to_handle_id_;
  ClientDiscardableManager* discardable_manager_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_CLIENT_DISCARDABLE_MANAGER_H_
