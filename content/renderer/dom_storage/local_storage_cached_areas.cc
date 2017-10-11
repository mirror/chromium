// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_cached_areas.h"

#include "content/common/dom_storage/dom_storage_types.h"
#include "content/renderer/dom_storage/local_storage_cached_area.h"

namespace content {

LocalStorageCachedAreas::LocalStorageCachedAreas(
    mojom::StoragePartitionService* storage_partition_service)
    : storage_partition_service_(storage_partition_service) {}

LocalStorageCachedAreas::~LocalStorageCachedAreas() {
}

scoped_refptr<LocalStorageCachedArea> LocalStorageCachedAreas::GetCachedArea(
    const url::Origin& origin) {
  AreaKey key(kLocalStorageNamespaceId, origin);
  if (cached_areas_.find(key) == cached_areas_.end()) {
    cached_areas_[key] = new LocalStorageCachedArea(
        origin, storage_partition_service_, this);
  }

  return make_scoped_refptr(cached_areas_[key]);
}

scoped_refptr<LocalStorageCachedArea> LocalStorageCachedAreas::GetSessionStorageArea(
    int64_t namespace_id,
    const url::Origin& origin) {
  DCHECK_NE(kLocalStorageNamespaceId, kInvalidSessionStorageNamespaceId);
  AreaKey key(namespace_id, origin);
  if (cached_areas_.find(key) == cached_areas_.end()) {
    cached_areas_[key] = new LocalStorageCachedArea(
        namespace_id, origin, storage_partition_service_, this);
  }

  return make_scoped_refptr(cached_areas_[key]);
}

void LocalStorageCachedAreas::CacheAreaClosed(
    LocalStorageCachedArea* cached_area) {
  AreaKey key(cached_area->namespace_id(), cached_area->origin());
  DCHECK(cached_areas_.find(key) != cached_areas_.end());
  cached_areas_.erase(key);
}

}  // namespace content
