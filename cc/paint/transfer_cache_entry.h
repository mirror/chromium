// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_TRANSFER_CACHE_ENTRY_H_
#define CC_PAINT_TRANSFER_CACHE_ENTRY_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include "cc/paint/paint_export.h"
#include "gpu/command_buffer/common/discardable_handle.h"

namespace cc {

using TransferCacheEntryId = gpu::ClientDiscardableHandle::Id;

// If you add an element here, make sure to update transfer_cache_entry.cc.
enum class TransferCacheEntryType { kRawMemory };

class CC_PAINT_EXPORT ClientTransferCacheEntry {
 public:
  virtual ~ClientTransferCacheEntry() {}
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t SerializedSize() const = 0;
  virtual bool Serialize(size_t size, uint8_t* data) const = 0;
};

class CC_PAINT_EXPORT ServiceTransferCacheEntry {
 public:
  static std::unique_ptr<ServiceTransferCacheEntry> Create(
      TransferCacheEntryType type);
  template <typename T>
  static TransferCacheEntryType DeduceType();

  virtual ~ServiceTransferCacheEntry() {}
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t Size() const = 0;
  virtual bool Deserialize(size_t size, uint8_t* data) = 0;
};

};  // namespace cc

#endif  // CC_PAINT_TRANSFER_CACHE_ENTRY_H_
