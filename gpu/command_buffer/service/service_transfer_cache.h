// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SERVICE_TRANSFER_CACHE_H_
#define GPU_COMMAND_BUFFER_SERVICE_SERVICE_TRANSFER_CACHE_H_

#include <vector>

#include "base/containers/mru_cache.h"
#include "gpu/command_buffer/common/discardable_handle.h"
#include "gpu/command_buffer/common/transfer_cache_entry.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/gpu_export.h"

namespace gpu {
class GPU_EXPORT ServiceTransferCache {
 public:
  ServiceTransferCache();
  ~ServiceTransferCache();

  void CreateLockedEntry(TransferCacheEntryId id,
                         ServiceDiscardableHandle handle,
                         TransferCacheEntryType type,
                         scoped_refptr<gpu::Buffer> buffer);

  void UnlockEntry(TransferCacheEntryId id);

  // Called when a texture is deleted, to clean up state.
  void DeleteEntry(TransferCacheEntryId id);

  // Specialized in CC file for supported types.
  template <typename T>
  T* GetEntry(TransferCacheEntryId id);

 private:
  void EnforceLimits();

  using EntryCache =
      base::MRUCache<TransferCacheEntryId,
                     std::pair<ServiceDiscardableHandle,
                               std::unique_ptr<ServiceTransferCacheEntry>>>;
  EntryCache entries_;

  // Total size of all |entries_|. The same as summing
  // GpuDiscardableEntry::size for each entry.
  size_t total_size_ = 0;

  // The limit above which the cache will start evicting resources.
  size_t cache_size_limit_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ServiceTransferCache);
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SERVICE_TRANSFER_CACHE_H_
