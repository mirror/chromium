// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_CLIENT_TRANSFER_CACHE_H_
#define GPU_COMMAND_BUFFER_CLIENT_CLIENT_TRANSFER_CACHE_H_

#include "cc/paint/transfer_cache_entry.h"
#include "gpu/command_buffer/client/client_discardable_manager.h"
#include "gpu/command_buffer/client/gles2_cmd_helper.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/mapped_memory.h"

namespace gpu {

class GPU_EXPORT ClientTransferCache {
 public:
  ClientTransferCache();
  ~ClientTransferCache();

  cc::TransferCacheEntryId CreateCacheEntry(
      gles2::GLES2Interface* gl,
      CommandBuffer* command_buffer,
      std::unique_ptr<cc::ClientTransferCacheEntry> entry);
  bool LockTransferCacheEntry(cc::TransferCacheEntryId id);
  void UnlockTransferCacheEntry(gles2::GLES2Interface* gl,
                                cc::TransferCacheEntryId id);
  void DeleteTransferCacheEntry(gles2::GLES2Interface* gl,
                                cc::TransferCacheEntryId id);

  // Test only functions
  ClientDiscardableManager* DiscardableManagerForTesting() {
    return &discardable_manager_;
  }

 private:
  ClientDiscardableManager discardable_manager_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_CLIENT_TRANSFER_CACHE_H_