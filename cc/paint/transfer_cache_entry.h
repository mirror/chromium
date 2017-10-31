// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_TRANSFER_CACHE_ENTRY_H_
#define CC_PAINT_TRANSFER_CACHE_ENTRY_H_

#include <cstddef>
#include <cstdint>

#include "cc/paint/paint_export.h"

#define ALL_TRANSFER_CACHE_TYPES                                     \
  TRANSFER_CACHE_TYPE(kRawMemory, ClientRawMemoryTransferCacheEntry, \
                      ServiceRawMemoryTransferCacheEntry)

namespace cc {

using TransferCacheEntryId = uint64_t;

enum class TransferCacheEntryType {
#define TRANSFER_CACHE_TYPE(enum_name, client_type, service_type) enum_name,
  ALL_TRANSFER_CACHE_TYPES
#undef TRANSFER_CACHE_TYPE
};

class CC_PAINT_EXPORT ClientTransferCacheEntry {
 public:
  virtual ~ClientTransferCacheEntry() {}
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t SerializedSize() const = 0;
  virtual bool Serialize(size_t size, uint8_t* data) const = 0;
};

class CC_PAINT_EXPORT ServiceTransferCacheEntry {
 public:
  virtual ~ServiceTransferCacheEntry() {}
  virtual TransferCacheEntryType Type() const = 0;
  virtual size_t Size() const = 0;
  virtual bool Deserialize(size_t size, uint8_t* data) = 0;
};

};  // namespace cc

#endif  // CC_PAINT_TRANSFER_CACHE_ENTRY_H_