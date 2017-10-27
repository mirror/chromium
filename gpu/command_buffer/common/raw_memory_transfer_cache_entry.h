// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_RAW_MEMORY_TRANSFER_CACHE_ENTRY_H_
#define GPU_COMMAND_BUFFER_COMMON_RAW_MEMORY_TRANSFER_CACHE_ENTRY_H_

#include "gpu/command_buffer/common/transfer_cache_entry.h"

namespace gpu {

class GPU_EXPORT RawMemoryTransferCacheEntry
    : public ClientTransferCacheEntry,
      public ServiceTransferCacheEntry {
 public:
  RawMemoryTransferCacheEntry();
  ~RawMemoryTransferCacheEntry() override;
  explicit RawMemoryTransferCacheEntry(std::vector<uint8_t> data);
  TransferCacheEntryType Type() const override;
  size_t DataSize() const override;
  size_t Size() const override;
  bool Serialize(size_t size, uint8_t* data) const override;
  bool Deserialize(size_t size, uint8_t* data) override;

 private:
  std::vector<uint8_t> data_;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_COMMON_RAW_MEMORY_TRANSFER_CACHE_ENTRY_H_