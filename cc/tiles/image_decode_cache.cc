// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_cache.h"

#include "base/atomic_sequence_num.h"
#include "base/containers/flat_map.h"
#include "base/lazy_instance.h"
#include "base/metrics/histogram_macros.h"
#include "cc/raster/tile_task.h"

namespace cc {
namespace {

base::AtomicSequenceNumber s_cache_id_;

}  // namespace

class ImageDecodeCache::Manager {
 public:
  Manager() = default;
  ~Manager() = default;

  void RegisterCache(ImageDecodeCache* cache) {
    base::AutoLock lock(lock_);
    cache_map_[cache->cache_id_] = cache;
  }

  void UnregisterCache(ImageDecodeCache* cache) {
    base::AutoLock lock(lock_);
    auto it = cache_map_.find(cache->cache_id_);
    DCHECK(it != cache_map_.end());
    cache_map_.erase(it);
  }

  void RegisterPurgeCallback(PaintImage::ContentId content_id,
                             const PaintImage& paint_image,
                             CacheId cache_id) {
    // It is safe to use base::Unretained here since the Manager is a leaky
    // LazyInstance.
    paint_image.RegisterPurgeCallback(
        content_id, base::BindOnce(&Manager::NotifyCache,
                                   base::Unretained(this), cache_id));
  }

 private:
  void NotifyCache(CacheId cache_id, PaintImage::ContentId content_id) {
    base::AutoLock lock(lock_);
    auto it = cache_map_.find(cache_id);
    if (it == cache_map_.end())
      return;

    it->second->OnPurgeDecodeForContentId(content_id);
  }

  base::Lock lock_;
  base::flat_map<ImageDecodeCache::CacheId, ImageDecodeCache*> cache_map_;
};

base::LazyInstance<ImageDecodeCache::Manager>::Leaky g_decode_cache_manager;

ImageDecodeCache::TaskResult::TaskResult(bool need_unref)
    : need_unref(need_unref) {}

ImageDecodeCache::TaskResult::TaskResult(scoped_refptr<TileTask> task)
    : task(std::move(task)), need_unref(true) {}

ImageDecodeCache::TaskResult::TaskResult(const TaskResult& result) = default;

ImageDecodeCache::TaskResult::~TaskResult() = default;

ImageDecodeCache::ImageDecodeCache() : cache_id_(s_cache_id_.GetNext()) {
  g_decode_cache_manager.Get().RegisterCache(this);
}

ImageDecodeCache::~ImageDecodeCache() {
  g_decode_cache_manager.Get().UnregisterCache(this);
}

void ImageDecodeCache::RecordImageMipLevelUMA(int mip_level) {
  DCHECK_GE(mip_level, 0);
  DCHECK_LT(mip_level, 32);
  UMA_HISTOGRAM_EXACT_LINEAR("Renderer4.ImageDecodeMipLevel", mip_level + 1,
                             33);
}

void ImageDecodeCache::RegisterPurgeCallback(PaintImage::ContentId content_id,
                                             const PaintImage& paint_image) {
  g_decode_cache_manager.Get().RegisterPurgeCallback(content_id, paint_image,
                                                     cache_id_);
}

}  // namespace cc
