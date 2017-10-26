// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_CLIENT_TRANSFER_CACHE_H_
#define GPU_COMMAND_BUFFER_CLIENT_CLIENT_TRANSFER_CACHE_H_

#include "gpu/command_buffer/client/client_discardable_manager.h"
#include "gpu/command_buffer/client/mapped_memory.h"
#include "gpu/command_buffer/common/transfer_cache_entry.h"

namespace gpu {

class GPU_EXPORT ClientTransferCache {
 public:
  ClientTransferCache(gles2::GLES2CmdHelper* helper,
                      MappedMemoryManager* manager);

  TransferCacheEntryId CreateCacheEntry(
      std::unique_ptr<ClientTransferCacheEntry> entry);
  bool LockTransferCacheEntry(TransferCacheEntryId id);
  void UnlockTransferCacheEntry(TransferCacheEntryId id);
  void DeleteTransferCacheEntry(TransferCacheEntryId id);

 private:
  gles2::GLES2CmdHelper* const helper_;
  MappedMemoryManager* const manager_;

  ClientDiscardableManager* discardable_manager_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_CLIENT_TRANSFER_CACHE_H_