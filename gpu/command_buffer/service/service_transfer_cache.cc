// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/service_transfer_cache.h"
#include "gpu/command_buffer/common/raw_memory_transfer_cache_entry.h"

#include "base/sys_info.h"

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

void ServiceTransferCache::CreateLockedEntry(
    TransferCacheEntryId id,
    ServiceDiscardableHandle handle,
    TransferCacheEntryType type,
    scoped_refptr<gpu::Buffer> buffer) {
  auto found = entries_.Peek(id);
  if (found != entries_.end()) {
    // We have somehow initialized an entry twice. The client *shouldn't* send
    // this command, but if it does, we will clean up the old entry and use
    // the new one.
    total_size_ -= found->second.second->Size();
    entries_.Erase(found);
  }

  std::unique_ptr<ServiceTransferCacheEntry> entry;
  switch (type) {
#define TRANSFER_CACHE_TYPE(type_name, entry_type) \
  case TransferCacheEntryType::type_name:          \
    entry.reset(new entry_type());                 \
    break;
    ALL_TRANSFER_CACHE_TYPES
#undef TRANSFER_CACHE_TYPE
  }

  if (!entry)
    return;

  entry->Deserialize(buffer->size(),
                     reinterpret_cast<uint8_t*>(buffer->memory()));
  total_size_ += entry->Size();
  entries_.Put(id, std::make_pair(handle, std::move(entry)));
  EnforceLimits();
}

void ServiceTransferCache::UnlockEntry(TransferCacheEntryId id) {
  auto found = entries_.Peek(id);
  if (found == entries_.end())
    return;

  found->second.first.Unlock();
}

void ServiceTransferCache::DeleteEntry(TransferCacheEntryId id) {
  auto found = entries_.Peek(id);
  if (found == entries_.end())
    return;

  found->second.first.ForceDelete();
  total_size_ -= found->second.second->Size();
  entries_.Erase(found);
}

#define TRANSFER_CACHE_TYPE(type_name, element_type)                       \
  template <>                                                              \
  element_type* ServiceTransferCache::GetEntry<element_type>(              \
      TransferCacheEntryId id) {                                           \
    auto found = entries_.Get(id);                                         \
    if (found == entries_.end())                                           \
      return nullptr;                                                      \
    if (found->second.second->Type() != TransferCacheEntryType::type_name) \
      return nullptr;                                                      \
    return static_cast<element_type*>(found->second.second.get());         \
  }
ALL_TRANSFER_CACHE_TYPES
#undef TRANSFER_CACHE_TYPE

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
