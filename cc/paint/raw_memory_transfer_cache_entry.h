// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_RAW_MEMORY_TRANSFER_CACHE_ENTRY_H_
#define CC_PAINT_RAW_MEMORY_TRANSFER_CACHE_ENTRY_H_

#include "cc/paint/transfer_cache_entry.h"

namespace cc {

class CC_PAINT_EXPORT RawMemoryTransferCacheEntry
    : public ClientTransferCacheEntry,
      public ServiceTransferCacheEntry {
 public:
  RawMemoryTransferCacheEntry();
  ~RawMemoryTransferCacheEntry() override;
  explicit RawMemoryTransferCacheEntry(std::vector<uint8_t> data);
  TransferCacheEntryType Type() const override;
  size_t SerializedSize() const override;
  size_t Size() const override;
  bool Serialize(size_t size, uint8_t* data) const override;
  bool Deserialize(size_t size, uint8_t* data) override;

 private:
  std::vector<uint8_t> data_;
};

}  // namespace cc

#endif  // CC_PAINT_RAW_MEMORY_TRANSFER_CACHE_ENTRY_H_