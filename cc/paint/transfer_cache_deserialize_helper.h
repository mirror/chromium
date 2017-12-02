// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_TRANSFER_CACHE_DESERIALIZE_HELPER_H_
#define CC_PAINT_TRANSFER_CACHE_DESERIALIZE_HELPER_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include "cc/paint/paint_export.h"
#include "cc/paint/transfer_cache_entry.h"

namespace cc {

class CC_PAINT_EXPORT TransferCacheDeserializeHelper {
 public:
  virtual ~TransferCacheDeserializeHelper() {}

  template <typename T>
  T* GetEntry(uint64_t id) {
    ServiceTransferCacheEntry* entry = GetEntryInternal(id);
    if (entry == nullptr) {
      return nullptr;
    }

    if (entry->Type() != ServiceTransferCacheEntry::DeduceType<T>())
      return nullptr;
    return static_cast<T*>(entry);
  }

 private:
  virtual ServiceTransferCacheEntry* GetEntryInternal(uint64_t id) = 0;
};

};  // namespace cc

#endif  // CC_PAINT_TRANSFER_CACHE_DESERIALIZE_HELPER_H_
