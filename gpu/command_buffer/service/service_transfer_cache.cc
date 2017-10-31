// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/service_transfer_cache.h"

#include "base/sys_info.h"
#include "cc/paint/raw_memory_transfer_cache_entry.h"

namespace gpu {
namespace {
size_t CacheSizeLimit() {
  if (base::SysInfo::IsLowEndDevice()) {
    return 4 * 1024 * 1024;
  } else {
    return 128 * 1024 * 1024;
  }
}

}  // namespace

ServiceTransferCache::ServiceTransferCache()
    : entries_(EntryCache::NO_AUTO_EVICT),
      cache_size_limit_(CacheSizeLimit()) {}

ServiceTransferCache::~ServiceTransferCache() = default;

bool ServiceTransferCache::CreateLockedEntry(cc::TransferCacheEntryId id,
                                             ServiceDiscardableHandle handle,
                                             cc::TransferCacheEntryType type,
                                             void* data_memory,
                                             size_t data_size) {
  auto found = entries_.Peek(id);
  if (found != entries_.end()) {
    // We have somehow initialized an entry twice. The client *shouldn't* send
    // this command, but if it does, we will clean up the old entry and use
    // the new one.
    total_size_ -= found->second.second->Size();
    entries_.Erase(found);
  }

  std::unique_ptr<cc::ServiceTransferCacheEntry> entry =
      cc::ServiceTransferCacheEntry::Create(type);
  if (!entry)
    return false;

  entry->Deserialize(data_size, reinterpret_cast<uint8_t*>(data_memory));
  total_size_ += entry->Size();
  entries_.Put(id, std::make_pair(handle, std::move(entry)));
  EnforceLimits();
  return true;
}

void ServiceTransferCache::UnlockEntry(cc::TransferCacheEntryId id) {
  auto found = entries_.Peek(id);
  if (found == entries_.end())
    return;

  found->second.first.Unlock();
}

void ServiceTransferCache::DeleteEntry(cc::TransferCacheEntryId id) {
  auto found = entries_.Peek(id);
  if (found == entries_.end())
    return;

  found->second.first.ForceDelete();
  total_size_ -= found->second.second->Size();
  entries_.Erase(found);
}

void ServiceTransferCache::EnforceLimits() {
  for (auto it = entries_.rbegin(); it != entries_.rend();) {
    if (total_size_ <= cache_size_limit_) {
      return;
    }
    if (!it->second.first.Delete()) {
      ++it;
      continue;
    }

    total_size_ -= it->second.second->Size();
    it = entries_.Erase(it);
  }
}

}  // namespace gpu
