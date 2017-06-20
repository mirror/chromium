// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/service_discardable_manager.h"

#include "base/memory/singleton.h"
#include "base/sys_info.h"
#include "base/numerics/safe_conversions.h"
#include "gpu/command_buffer/service/texture_manager.h"

namespace gpu {
ServiceDiscardableManager::GpuDiscardableEntry::GpuDiscardableEntry(
    ServiceDiscardableHandle handle,
    size_t size)
    : handle(handle), size(size) {}
ServiceDiscardableManager::GpuDiscardableEntry::GpuDiscardableEntry(
    const GpuDiscardableEntry& other) = default;
ServiceDiscardableManager::GpuDiscardableEntry::GpuDiscardableEntry(
    GpuDiscardableEntry&& other) = default;
ServiceDiscardableManager::GpuDiscardableEntry::~GpuDiscardableEntry() =
    default;

ServiceDiscardableManager::ServiceDiscardableManager()
    : entries_(EntryCache::NO_AUTO_EVICT) {
  // Calculate our base values for min/max cache size and growth. These values
  // are designed to roughly match the existing image cache limits. These will
  // be updated as more types of data are moved to this cache.

  // Threshold at which we move from a normal cache to a high-memory cache.
  const int kHighMemoryCacheSizeMemoryThresholdMB = 4 * 1024;

  // Values for low-end Android devices. Low end assumes one renderer and does
  // not grow the cache size.
  const size_t kLowEndMinCacheSizeBytes = 2 * 1024 * 1024;
  const size_t kLowEndMaxCacheSizeBytes = 2 * 1024 * 1024;
  const size_t kLowEndCacheGrowthPerClientBytes = 0;

  // Values for normal devices.
  const size_t kNormalMinCacheSizeBytes = 128 * 1024 * 1024;
  const size_t kNormalMaxCacheSizeBytes = 384 * 1024 * 1024;
  const size_t kNormalCacheGrowthPerClientBytes = kNormalMinCacheSizeBytes;

  // Values for high-memory devices.
  const size_t kHighMemoryMinCacheSizeBytes = 256 * 1024 * 1024;
  const size_t kHighMemoryMaxCacheSizeBytes = 512 * 1024 * 1024;
  const size_t kHighMemoryCacheGrowthPerClientBytes = 128 * 1024 * 1024;

  if (base::SysInfo::IsLowEndDevice()) {
    min_cache_size_bytes_ = kLowEndMinCacheSizeBytes;
    max_cache_size_bytes_ = kLowEndMaxCacheSizeBytes;
    cache_growth_per_client_bytes_ = kLowEndCacheGrowthPerClientBytes;
  } else if (base::SysInfo::AmountOfPhysicalMemoryMB() <
             kHighMemoryCacheSizeMemoryThresholdMB) {
    min_cache_size_bytes_ = kNormalMinCacheSizeBytes;
    max_cache_size_bytes_ = kNormalMaxCacheSizeBytes;
    cache_growth_per_client_bytes_ = kNormalCacheGrowthPerClientBytes;
  } else {
    min_cache_size_bytes_ = kHighMemoryMinCacheSizeBytes;
    max_cache_size_bytes_ = kHighMemoryMaxCacheSizeBytes;
    cache_growth_per_client_bytes_ = kHighMemoryCacheGrowthPerClientBytes;
  }
}

ServiceDiscardableManager::~ServiceDiscardableManager() {
#if DCHECK_IS_ON()
  for (const auto& entry : entries_) {
    DCHECK(nullptr == entry.second.unlocked_texture_ref);
  }
#endif
}

void ServiceDiscardableManager::InsertLockedTexture(
    uint32_t texture_id,
    size_t texture_size,
    gles2::TextureManager* texture_manager,
    ServiceDiscardableHandle handle) {
  auto found = entries_.Get({texture_id, texture_manager});
  if (found != entries_.end()) {
    // We have somehow initialized a texture twice. The client *shouldn't* send
    // this command, but if it does, we will clean up the old entry and use
    // the new one.
    total_size_ -= found->second.size;
    if (found->second.unlocked_texture_ref) {
      texture_manager->ReturnTexture(
          std::move(found->second.unlocked_texture_ref));
    }
    entries_.Erase(found);
  }

  total_size_ += texture_size;
  entries_.Put({texture_id, texture_manager},
               GpuDiscardableEntry{handle, texture_size});
  clients_.insert(texture_manager);
  EnforceLimits();
}

bool ServiceDiscardableManager::UnlockTexture(
    uint32_t texture_id,
    gles2::TextureManager* texture_manager,
    gles2::TextureRef** texture_to_unbind) {
  *texture_to_unbind = nullptr;

  auto found = entries_.Get({texture_id, texture_manager});
  if (found == entries_.end())
    return false;

  found->second.handle.Unlock();
  if (--found->second.service_ref_count_ == 0) {
    found->second.unlocked_texture_ref =
        texture_manager->TakeTexture(texture_id);
    *texture_to_unbind = found->second.unlocked_texture_ref.get();
  }

  return true;
}

bool ServiceDiscardableManager::LockTexture(
    uint32_t texture_id,
    gles2::TextureManager* texture_manager) {
  auto found = entries_.Peek({texture_id, texture_manager});
  if (found == entries_.end())
    return false;

  ++found->second.service_ref_count_;
  if (found->second.unlocked_texture_ref) {
    texture_manager->ReturnTexture(
        std::move(found->second.unlocked_texture_ref));
  }

  return true;
}

void ServiceDiscardableManager::OnTextureManagerDestruction(
    gles2::TextureManager* texture_manager) {
  clients_.erase(texture_manager);

  for (auto& entry : entries_) {
    if (entry.first.texture_manager == texture_manager &&
        entry.second.unlocked_texture_ref) {
      texture_manager->ReturnTexture(
          std::move(entry.second.unlocked_texture_ref));
    }
  }

  EnforceLimits();
}

void ServiceDiscardableManager::OnTextureDeleted(
    uint32_t texture_id,
    gles2::TextureManager* texture_manager) {
  auto found = entries_.Get({texture_id, texture_manager});
  if (found == entries_.end())
    return;

  found->second.handle.ForceDelete();
  total_size_ -= found->second.size;
  entries_.Erase(found);
}

void ServiceDiscardableManager::OnTextureSizeChanged(
    uint32_t texture_id,
    gles2::TextureManager* texture_manager,
    size_t new_size) {
  auto found = entries_.Get({texture_id, texture_manager});
  if (found == entries_.end())
    return;

  total_size_ -= found->second.size;
  found->second.size = new_size;
  total_size_ += found->second.size;

  EnforceLimits();
}

void ServiceDiscardableManager::EnforceLimits() {
  size_t limit_bytes = CacheSizeLimitBytes();

  for (auto it = entries_.rbegin(); it != entries_.rend();) {
    if (total_size_ <= limit_bytes) {
      return;
    }
    if (!it->second.handle.Delete()) {
      ++it;
      continue;
    }

    total_size_ -= it->second.size;

    gles2::TextureManager* texture_manager = it->first.texture_manager;
    uint32_t texture_id = it->first.texture_id;

    // While unlocked, we hold the texture ref. Return this to the texture
    // manager for cleanup.
    texture_manager->ReturnTexture(std::move(it->second.unlocked_texture_ref));

    // Erase before calling texture_manager->RemoveTexture, to avoid attempting
    // to remove the texture from entries_ twice.
    it = entries_.Erase(it);
    texture_manager->RemoveTexture(texture_id);
  }
}

size_t ServiceDiscardableManager::CacheSizeLimitBytes() const {
  base::CheckedNumeric<size_t> limit_checked(min_cache_size_bytes_);
  base::CheckedNumeric<size_t> growth_checked(cache_growth_per_client_bytes_);
  growth_checked *= std::max<size_t>(1, clients_.size()) - 1;
  limit_checked += growth_checked;

  size_t limit = limit_checked.ValueOrDefault(max_cache_size_bytes_);
  return std::min(max_cache_size_bytes_, limit);
}

bool ServiceDiscardableManager::IsEntryLockedForTesting(
    uint32_t texture_id,
    gles2::TextureManager* texture_manager) const {
  auto found = entries_.Peek({texture_id, texture_manager});
  DCHECK(found != entries_.end());

  return found->second.handle.IsLockedForTesting();
}

gles2::TextureRef* ServiceDiscardableManager::UnlockedTextureRefForTesting(
    uint32_t texture_id,
    gles2::TextureManager* texture_manager) const {
  auto found = entries_.Peek({texture_id, texture_manager});
  DCHECK(found != entries_.end());

  return found->second.unlocked_texture_ref.get();
}

}  // namespace gpu
