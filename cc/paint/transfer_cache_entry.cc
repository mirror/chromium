// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/transfer_cache_entry.h"

#include "base/memory/ptr_util.h"
#include "cc/paint/raw_memory_transfer_cache_entry.h"

namespace cc {

std::unique_ptr<ServiceTransferCacheEntry> ServiceTransferCacheEntry::Create(
    TransferCacheEntryType type) {
  switch (type) {
    case TransferCacheEntryType::kRawMemory:
      return base::MakeUnique<ServiceRawMemoryTransferCacheEntry>();
  }
}

template <>
TransferCacheEntryType
ServiceTransferCacheEntry::DeduceType<ServiceRawMemoryTransferCacheEntry>() {
  return TransferCacheEntryType::kRawMemory;
}

}  // namespace cc
