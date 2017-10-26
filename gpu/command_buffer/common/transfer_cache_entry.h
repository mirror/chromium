// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_COMMON_TRANSFER_CACHE_ENTRY_H_
#define GPU_COMMAND_BUFFER_COMMON_TRANSFER_CACHE_ENTRY_H_

#include <cstddef>
#include <cstdint>

#include "gpu/gpu_export.h"

namespace gpu {

using TransferCacheEntryId = uint64_t;

enum class TransferCacheEntryType { kRawMemory };

class GPU_EXPORT ClientTransferCacheEntry {
 public:
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t DataSize() const = 0;
  virtual bool Serialize(size_t size, uint8_t* data) const = 0;
};

class GPU_EXPORT ServiceTransferCacheEntry {
 public:
  virtual bool Deserialize(size_t size, uint8_t* data) = 0;
};

};  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_COMMON_TRANSFER_CACHE_ENTRY_H_