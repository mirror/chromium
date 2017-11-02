// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SERVICE_TRANSFER_CACHE_H_
#define GPU_COMMAND_BUFFER_SERVICE_SERVICE_TRANSFER_CACHE_H_

#include <vector>

#include "base/containers/mru_cache.h"
#include "cc/paint/transfer_cache_entry.h"
#include "gpu/command_buffer/common/discardable_handle.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/gpu_export.h"

namespace gpu {

// ServiceTransferCache is a GPU process interface for retreiving cached entries
// from the transfer cache. These entries are populated by client calls to the
// ClientTransferCache.
//
// In addition to access, the ServiceTransferCache is also responsible for
// unlocking and deleting entries when no longer needed, as well as enforcing
// cache limits. If the cache exceeds its specified limits, unlocked transfer
// cache entries may be deleted.
class GPU_EXPORT ServiceTransferCache {
 public:
  ServiceTransferCache();
  ~ServiceTransferCache();

  bool CreateLockedEntry(cc::TransferCacheEntryId id,
                         ServiceDiscardableHandle handle,
                         cc::TransferCacheEntryType type,
                         void* data_memory,
                         size_t data_size);

  void UnlockEntry(cc::TransferCacheEntryId id);
  void DeleteEntry(cc::TransferCacheEntryId id);
  cc::ServiceTransferCacheEntry* GetEntry(cc::TransferCacheEntryId id);

 private:
  void EnforceLimits();

  struct CacheEntryInternal {
    CacheEntryInternal(ServiceDiscardableHandle handle,
                       std::unique_ptr<cc::ServiceTransferCacheEntry> entry);
    CacheEntryInternal(CacheEntryInternal&& other);
    CacheEntryInternal& operator=(CacheEntryInternal&& other);
    ~CacheEntryInternal();
    ServiceDiscardableHandle handle;
    std::unique_ptr<cc::ServiceTransferCacheEntry> entry;
  };
  using EntryCache =
      base::MRUCache<cc::TransferCacheEntryId, CacheEntryInternal>;
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
