// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_cached_areas.h"

#include "base/sys_info.h"
#include "content/renderer/dom_storage/local_storage_cached_area.h"

namespace content {
namespace {
const size_t kTotalCacheLimitInBytesLowEnd = 1 * 1024 * 1024;
const size_t kTotalCacheLimitInBytes = 10 * 1024 * 1024;
}  // namespace

LocalStorageCachedAreas::LocalStorageCachedAreas(
    mojom::StoragePartitionService* storage_partition_service)
    : storage_partition_service_(storage_partition_service),
      total_cache_limit_(base::SysInfo::AmountOfPhysicalMemoryMB() < 1024
                             ? kTotalCacheLimitInBytesLowEnd
                             : kTotalCacheLimitInBytes) {}

LocalStorageCachedAreas::~LocalStorageCachedAreas() {
}

scoped_refptr<LocalStorageCachedArea> LocalStorageCachedAreas::GetCachedArea(
    const url::Origin& origin) {
  auto it = cached_areas_.find(origin);
  if (cached_areas_.find(origin) == cached_areas_.end()) {
    ClearAreasIfNeeded();
    it = cached_areas_
             .emplace(std::piecewise_construct,
                      std::forward_as_tuple(url::Origin(origin)),
                      std::forward_as_tuple(
                          new LocalStorageCachedArea(
                              origin, storage_partition_service_, this),
                          1))
             .first;
  } else {
    it->second.open_count++;
  }
  return it->second.area;
}

void LocalStorageCachedAreas::CacheAreaClosed(
    LocalStorageCachedArea* cached_area) {
  auto it = cached_areas_.find(cached_area->origin());
  DCHECK(it != cached_areas_.end());
  it->second.open_count--;
}

void LocalStorageCachedAreas::ClearAreasIfNeeded() {
  while (TotalCacheSize() > total_cache_limit_) {
    auto it = cached_areas_.begin();
    for (; it != cached_areas_.end(); ++it) {
      if (!it->second.open_count) {
        cached_areas_.erase(it);
        break;
      }
    }
    if (it == cached_areas_.end())
      return;
  }
}

size_t LocalStorageCachedAreas::TotalCacheSize() const {
  size_t total = 0;
  for (const auto& it : cached_areas_)
    total += it.second.area->area_memory_used();
  return total;
}

LocalStorageCachedAreas::AreaHolder::AreaHolder(LocalStorageCachedArea* area,
                                                int open_count)
    : area(make_scoped_refptr(area)), open_count(open_count) {}

LocalStorageCachedAreas::AreaHolder::~AreaHolder() {}

}  // namespace content
