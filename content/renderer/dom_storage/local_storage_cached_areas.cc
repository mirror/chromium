// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_cached_areas.h"

#include "base/sys_info.h"
#include "content/renderer/dom_storage/local_storage_cached_area.h"

namespace content {
namespace {
const size_t kTotalCacheLimitInBytesLowEnd = 1 * 1024 * 1024;
const size_t kTotalCacheLimitInBytes = 5 * 1024 * 1024;
}  // namespace

LocalStorageCachedAreas::LocalStorageCachedAreas(
    mojom::StoragePartitionService* storage_partition_service)
    : storage_partition_service_(storage_partition_service),
      total_cache_limit_(base::SysInfo::AmountOfPhysicalMemoryMB() < 1024
                             ? kTotalCacheLimitInBytesLowEnd
                             : kTotalCacheLimitInBytes) {}

LocalStorageCachedAreas::~LocalStorageCachedAreas() {}

scoped_refptr<LocalStorageCachedArea> LocalStorageCachedAreas::GetCachedArea(
    const url::Origin& origin) {
  auto it = cached_areas_.find(origin);
  if (it == cached_areas_.end()) {
    ClearAreasIfNeeded();
    it = cached_areas_
             .emplace(origin, new LocalStorageCachedArea(
                                  origin, storage_partition_service_, this))
             .first;
  }
  return it->second;
}

size_t LocalStorageCachedAreas::TotalCacheSize() const {
  size_t total = 0;
  for (const auto& it : cached_areas_)
    total += it.second.get()->area_memory_used();
  return total;
}

void LocalStorageCachedAreas::ClearAreasIfNeeded() {
  size_t total_cache_size = TotalCacheSize();
  while (total_cache_size > total_cache_limit_) {
    auto it = cached_areas_.begin();
    for (; it != cached_areas_.end(); ++it) {
      if (it->second->HasOneRef()) {
        total_cache_size -= it->second->area_memory_used();
        cached_areas_.erase(it);
        break;
      }
    }
    if (it == cached_areas_.end())
      return;
  }
}

}  // namespace content
