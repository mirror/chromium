// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_SOFTWARE_IMAGE_DECODE_CACHE_UTILS_H_
#define CC_TILES_SOFTWARE_IMAGE_DECODE_CACHE_UTILS_H_

#include <cstddef>
#include <memory>

#include "base/memory/discardable_memory.h"
#include "base/memory/scoped_refptr.h"
#include "cc/cc_export.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/draw_image.h"
#include "cc/paint/paint_image.h"
#include "cc/raster/tile_task.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSize.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace cc {

class CC_EXPORT SoftwareImageDecodeCacheUtils {
 private:
  // The following should only be accessed by the software image cache.
  friend class SoftwareImageDecodeCache;

  // CacheKey is a class that gets a cache key out of a given draw
  // image. That is, this key uniquely identifies an image in the cache. Note
  // that it's insufficient to use SkImage's unique id, since the same image can
  // appear in the cache multiple times at different scales and filter
  // qualities.
  class CacheKey {
   public:
    // Enum indicating the type of processing to do for this key:
    // kOriginal - use the original decode without any subrecting or scaling.
    // kSubrectOriginal - extract a subrect from the original decode but do not
    //                    scale it.
    // kSubrectAndScale - extract a subrect (if needed) from the original decode
    //                    and scale it.
    enum ProcessingType { kOriginal, kSubrectOriginal, kSubrectAndScale };

    static CacheKey FromDrawImage(const DrawImage& image,
                                  SkColorType color_type);

    CacheKey(const CacheKey& other);

    bool operator==(const CacheKey& other) const {
      // The frame_key always has to be the same. However, after that all
      // original decodes are the same, so if we can use the original decode,
      // return true. If not, then we have to compare every field. Note we don't
      // compare |nearest_neighbor_| because we would only use kOriginal type in
      // that case (dchecked below), which implies no scale. The returned scale
      // to Skia would respect the nearest neighbor value of the requested
      // image.
      DCHECK(!is_nearest_neighbor_ || type_ == kOriginal);
      return frame_key_ == other.frame_key_ && type_ == other.type_ &&
             target_color_space_ == other.target_color_space_ &&
             (type_ == kOriginal || (src_rect_ == other.src_rect_ &&
                                     target_size_ == other.target_size_));
    }

    bool operator!=(const CacheKey& other) const { return !(*this == other); }

    const PaintImage::FrameKey& frame_key() const { return frame_key_; }
    ProcessingType type() const { return type_; }
    bool is_nearest_neighbor() const { return is_nearest_neighbor_; }
    gfx::Rect src_rect() const { return src_rect_; }
    gfx::Size target_size() const { return target_size_; }
    const gfx::ColorSpace& target_color_space() const {
      return target_color_space_;
    }

    size_t get_hash() const { return hash_; }

    // Helper to figure out how much memory the locked image represented by this
    // key would take.
    size_t locked_bytes() const {
      // TODO(vmpstr): Handle formats other than RGBA.
      base::CheckedNumeric<size_t> result = 4;
      result *= target_size_.width();
      result *= target_size_.height();
      return result.ValueOrDefault(std::numeric_limits<size_t>::max());
    }

    std::string ToString() const;

   private:
    CacheKey(PaintImage::FrameKey frame_key,
             ProcessingType type,
             bool is_nearest_neighbor,
             const gfx::Rect& src_rect,
             const gfx::Size& target_size,
             const gfx::ColorSpace& target_color_space);

    PaintImage::FrameKey frame_key_;
    ProcessingType type_;
    bool is_nearest_neighbor_;
    gfx::Rect src_rect_;
    gfx::Size target_size_;
    gfx::ColorSpace target_color_space_;
    size_t hash_;
  };

  struct CacheKeyHash {
    size_t operator()(const CacheKey& key) const { return key.get_hash(); }
  };

  class CacheEntry {
   public:
    enum class TaskType { kInRaster, kOutOfRaster };

    CacheEntry();
    CacheEntry(const SkImageInfo& info,
               std::unique_ptr<base::DiscardableMemory> memory,
               const SkSize& src_rect_offset);
    ~CacheEntry();

    bool Lock();
    void Unlock();
    void TakeMemoryFromEntry(CacheEntry* entry);

    // Usage functionality.
    void set_used() { usage_stats_.used = true; }
    void set_cached() { usage_stats_.cached = true; }
    void set_out_of_raster() { usage_stats_.first_lock_out_of_raster = true; }
    void set_decode_failed() { decode_failed_ = true; }
    void set_budgeted(bool flag) { is_budgeted_ = flag; }

    // State queries.
    bool decode_failed() const { return decode_failed_; }
    const SkSize& src_rect_offset() const { return src_rect_offset_; }
    uint64_t tracing_id() const { return tracing_id_; }
    bool is_locked() const { return is_locked_; }
    bool is_budgeted() const { return is_budgeted_; }
    const sk_sp<SkImage>& image() const { return image_; }
    base::DiscardableMemory* memory_for_tracing() { return memory_.get(); }

    // Ref counting.
    int ref() { return ++ref_count_; }
    int unref() { return --ref_count_; }
    bool has_refs() const { return ref_count_ != 0; }

    // Tasks associated with the entry.
    const scoped_refptr<TileTask>& task(TaskType type) const {
      return type == TaskType::kInRaster ? in_raster_task_
                                         : out_of_raster_task_;
    }
    scoped_refptr<TileTask>& task(TaskType type) {
      return const_cast<scoped_refptr<TileTask>&>(
          static_cast<const CacheEntry*>(this)->task(type));
    }

   private:
    struct UsageStats {
      // We can only create a decoded image in a locked state, so the initial
      // lock count is 1.
      int lock_count = 1;
      bool used = false;
      bool last_lock_failed = false;
      bool first_lock_wasted = false;
      bool first_lock_out_of_raster = false;
      // Indicates whether this entry was ever in the cache.
      bool cached = false;
    };

    void ResetMemory();

    sk_sp<SkImage> image_;
    int ref_count_ = 0;
    bool decode_failed_ = false;
    bool is_locked_ = false;
    bool is_budgeted_ = false;
    scoped_refptr<TileTask> in_raster_task_;
    scoped_refptr<TileTask> out_of_raster_task_;
    std::unique_ptr<base::DiscardableMemory> memory_;
    SkSize src_rect_offset_;
    uint64_t tracing_id_ = 0;
    UsageStats usage_stats_;
  };

  // Generate a cache entry from the given key and image. Returns nullptr in
  // case of failure.
  static std::unique_ptr<CacheEntry> Decode(const CacheKey& key,
                                            const PaintImage& image,
                                            SkColorType color_type);

  // Use the given decoded candidate to produce a cache entry for the key. This
  // involves scaling and possibly extracting a subset. The subset is extracted
  // if so requested by the caller.
  static std::unique_ptr<CacheEntry> ScaleCandidateEntry(
      const CacheKey& key,
      const CacheEntry* candidate,
      bool needs_extract_subset,
      SkColorType color_type);
};

}  // namespace cc

#endif  // CC_TILES_SOFTWARE_IMAGE_DECODE_CACHE_UTILS_H_
