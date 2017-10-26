// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_TRANSFER_CACHE_ENTRY_H_
#define GPU_COMMAND_BUFFER_COMMON_TRANSFER_CACHE_ENTRY_H_

#include <cstddef>
#include <cstdint>

#include <vector>

#include "gpu/gpu_export.h"

#define ALL_TRANSFER_CACHE_TYPES \
  TRANSFER_CACHE_TYPE(kRawMemory, RawMemoryTransferCacheEntry)

namespace gpu {

using TransferCacheEntryId = uint64_t;

enum class TransferCacheEntryType {
#define TRANSFER_CACHE_TYPE(type_name, entry_type) type_name,
  ALL_TRANSFER_CACHE_TYPES
#undef TRANSFER_CACHE_TYPE
};

class GPU_EXPORT ClientTransferCacheEntry {
 public:
  virtual ~ClientTransferCacheEntry() {}
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t DataSize() const = 0;
  virtual bool Serialize(size_t size, uint8_t* data) const = 0;
};

class GPU_EXPORT ServiceTransferCacheEntry {
 public:
  virtual ~ServiceTransferCacheEntry() {}
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t Size() const = 0;
  virtual bool Deserialize(size_t size, uint8_t* data) = 0;
};

};  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_COMMON_TRANSFER_CACHE_ENTRY_H_