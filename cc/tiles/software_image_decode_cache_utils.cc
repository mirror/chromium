// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/software_image_decode_cache_utils.h"
#include "base/atomic_sequence_num.h"
#include "base/hash.h"
#include "base/logging.h"
#include "base/memory/discardable_memory.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/tiles/mipmap_util.h"
#include "ui/gfx/skia_util.h"

namespace cc {
namespace {

// If the size of the original sized image breaches kMemoryRatioToSubrect but we
// don't need to scale the image, consider caching only the needed subrect.
// The second part that much be true is that we cache only the needed subrect if
// the total size needed for the subrect is at most kMemoryRatioToSubrect *
// (size needed for the full original image).
// Note that at least one of the dimensions has to be at least
// kMinDimensionToSubrect before an image can breach the threshold.
const size_t kMemoryThresholdToSubrect = 64 * 1024 * 1024;
const int kMinDimensionToSubrect = 4 * 1024;
const float kMemoryRatioToSubrect = 0.5f;

// Tracing ID sequence for use in CacheEntry.
base::AtomicSequenceNumber g_next_tracing_id_;

gfx::Rect GetSrcRect(const DrawImage& image) {
  const SkIRect& src_rect = image.src_rect();
  int x = std::max(0, src_rect.x());
  int y = std::max(0, src_rect.y());
  int right = std::min(image.paint_image().width(), src_rect.right());
  int bottom = std::min(image.paint_image().height(), src_rect.bottom());
  if (x >= right || y >= bottom)
    return gfx::Rect();
  return gfx::Rect(x, y, right - x, bottom - y);
}

}  // namespace

// CacheKey -------------------------------------------------
// static
SoftwareImageDecodeCacheUtils::CacheKey
SoftwareImageDecodeCacheUtils::CacheKey::FromDrawImage(const DrawImage& image,
                                                       SkColorType color_type) {
  const PaintImage::FrameKey frame_key = image.frame_key();

  const SkSize& scale = image.scale();
  // If the src_rect falls outside of the image, we need to clip it since
  // otherwise we might end up with uninitialized memory in the decode process.
  // Note that the scale is still unchanged and the target size is now a
  // function of the new src_rect.
  gfx::Rect src_rect = GetSrcRect(image);

  // Start with the exact target size. However, this will be adjusted below to
  // be either a mip level, the original size, or a subrect size. This is done
  // to keep memory accounting correct.
  gfx::Size target_size(
      SkScalarRoundToInt(std::abs(src_rect.width() * scale.width())),
      SkScalarRoundToInt(std::abs(src_rect.height() * scale.height())));

  // If the target size is empty, then we'll be skipping the decode anyway, so
  // the filter quality doesn't matter. Early out instead.
  if (target_size.IsEmpty()) {
    return SoftwareImageDecodeCacheUtils::CacheKey(frame_key, kOriginal, false,
                                                   src_rect, target_size,
                                                   image.target_color_space());
  }

  ProcessingType type = kOriginal;
  bool is_nearest_neighbor = image.filter_quality() == kNone_SkFilterQuality;
  int mip_level = MipMapUtil::GetLevelForSize(src_rect.size(), target_size);
  // If any of the following conditions hold, then use at most low filter
  // quality and adjust the target size to match the original image:
  // - Quality is none: We need a pixelated image, so we can't upgrade it.
  // - Format is 4444: Skia doesn't support scaling these, so use low
  //   filter quality.
  // - Mip level is 0: The required mip is the original image, so just use low
  //   filter quality.
  // - Matrix is not decomposable: There's perspective on this image and we
  //   can't determine the size, so use the original.
  if (is_nearest_neighbor || color_type == kARGB_4444_SkColorType ||
      mip_level == 0 || !image.matrix_is_decomposable()) {
    type = kOriginal;
    // Update the size to be the original image size.
    target_size =
        gfx::Size(image.paint_image().width(), image.paint_image().height());
  } else {
    type = kSubrectAndScale;
    // Update the target size to be a mip level size.
    // TODO(vmpstr): MipMapUtil and JPEG decoders disagree on what to do with
    // odd sizes. If width = 2k + 1, and the mip level is 1, then this will
    // return width = k; JPEG decoder, however, will support decoding to width =
    // k + 1. We need to figure out what to do in this case.
    SkSize mip_scale_adjustment =
        MipMapUtil::GetScaleAdjustmentForLevel(src_rect.size(), mip_level);
    target_size.set_width(src_rect.width() * mip_scale_adjustment.width());
    target_size.set_height(src_rect.height() * mip_scale_adjustment.height());
  }

  // If the original image is large, we might want to do a subrect instead if
  // the subrect would be kMemoryRatioToSubrect times smaller.
  if (type == kOriginal &&
      (image.paint_image().width() >= kMinDimensionToSubrect ||
       image.paint_image().height() >= kMinDimensionToSubrect)) {
    base::CheckedNumeric<size_t> checked_original_size = 4u;
    checked_original_size *= image.paint_image().width();
    checked_original_size *= image.paint_image().height();
    size_t original_size = checked_original_size.ValueOrDefault(
        std::numeric_limits<size_t>::max());

    base::CheckedNumeric<size_t> checked_src_rect_size = 4u;
    checked_src_rect_size *= src_rect.width();
    checked_src_rect_size *= src_rect.height();
    size_t src_rect_size = checked_src_rect_size.ValueOrDefault(
        std::numeric_limits<size_t>::max());

    // If the sizes are such that we get good savings by subrecting, then do
    // that. Also update the target size to be the src rect size since that's
    // the rect we want to use.
    if (original_size > kMemoryThresholdToSubrect &&
        src_rect_size <= original_size * kMemoryRatioToSubrect) {
      type = kSubrectOriginal;
      target_size = src_rect.size();
    }
  }

  if (type == kOriginal) {
    DCHECK(target_size == gfx::Size(image.paint_image().width(),
                                    image.paint_image().height()));
    src_rect = gfx::Rect(target_size);
  }

  return SoftwareImageDecodeCacheUtils::CacheKey(
      frame_key, type, is_nearest_neighbor, src_rect, target_size,
      image.target_color_space());
}

SoftwareImageDecodeCacheUtils::CacheKey::CacheKey(
    PaintImage::FrameKey frame_key,
    ProcessingType type,
    bool is_nearest_neighbor,
    const gfx::Rect& src_rect,
    const gfx::Size& target_size,
    const gfx::ColorSpace& target_color_space)
    : frame_key_(frame_key),
      type_(type),
      is_nearest_neighbor_(is_nearest_neighbor),
      src_rect_(src_rect),
      target_size_(target_size),
      target_color_space_(target_color_space) {
  if (type == kOriginal) {
    hash_ = frame_key_.hash();
  } else {
    // TODO(vmpstr): This is a mess. Maybe it's faster to just search the vector
    // always (forwards or backwards to account for LRU).
    uint64_t src_rect_hash = base::HashInts(
        static_cast<uint64_t>(base::HashInts(src_rect_.x(), src_rect_.y())),
        static_cast<uint64_t>(
            base::HashInts(src_rect_.width(), src_rect_.height())));

    uint64_t target_size_hash =
        base::HashInts(target_size_.width(), target_size_.height());

    hash_ = base::HashInts(base::HashInts(src_rect_hash, target_size_hash),
                           frame_key_.hash());
  }
  // Include the target color space in the hash regardless of scaling.
  hash_ = base::HashInts(hash_, target_color_space.GetHash());
}

SoftwareImageDecodeCacheUtils::CacheKey::CacheKey(
    const SoftwareImageDecodeCacheUtils::CacheKey& other) = default;

std::string SoftwareImageDecodeCacheUtils::CacheKey::ToString() const {
  std::ostringstream str;
  str << "frame_key[" << frame_key_.ToString() << "]\ntype[";
  switch (type_) {
    case kOriginal:
      str << "Original";
      break;
    case kSubrectOriginal:
      str << "SubrectOriginal";
      break;
    case kSubrectAndScale:
      str << "SubrectAndScale";
      break;
  }
  str << "]\nis_nearest_neightbor[" << is_nearest_neighbor_ << "]\nsrc_rect["
      << src_rect_.ToString() << "]\ntarget_size[" << target_size_.ToString()
      << "]\ntarget_color_space[" << target_color_space_.ToString()
      << "]\nhash[" << hash_ << "]";
  return str.str();
}

// CacheEntry ------------------------------------------------------------------
SoftwareImageDecodeCacheUtils::CacheEntry::CacheEntry()
    : tracing_id_(g_next_tracing_id_.GetNext()) {}
SoftwareImageDecodeCacheUtils::CacheEntry::CacheEntry(
    const SkImageInfo& info,
    std::unique_ptr<base::DiscardableMemory> memory,
    const SkSize& src_rect_offset)
    : is_locked_(true),
      memory_(std::move(memory)),
      src_rect_offset_(src_rect_offset),
      tracing_id_(g_next_tracing_id_.GetNext()) {
  DCHECK(memory_);
  SkPixmap pixmap(info, memory_->data(), info.minRowBytes());
  image_ = SkImage::MakeFromRaster(
      pixmap, [](const void* pixels, void* context) {}, nullptr);
}

SoftwareImageDecodeCacheUtils::CacheEntry::~CacheEntry() {
  DCHECK(!is_locked_);

  // We create temporary CacheEntries as a part of decoding. However, we move
  // the memory to cache entries that actually live in the cache. Destroying the
  // temporaries should not cause any of the stats to be recorded. Specifically,
  // if allowed to report, they would report every single temporary entry as
  // wasted, which is misleading. As a fix, don't report on a cache entry that
  // has never been in the cache.
  if (!usage_stats_.cached)
    return;

  // lock_count | used  | last lock failed | result state
  // ===========+=======+==================+==================
  //  1         | false | false            | WASTED
  //  1         | false | true             | WASTED
  //  1         | true  | false            | USED
  //  1         | true  | true             | USED_RELOCK_FAILED
  //  >1        | false | false            | WASTED_RELOCKED
  //  >1        | false | true             | WASTED_RELOCKED
  //  >1        | true  | false            | USED_RELOCKED
  //  >1        | true  | true             | USED_RELOCKED
  // Note that it's important not to reorder the following enums, since the
  // numerical values are used in the histogram code.
  enum State : int {
    DECODED_IMAGE_STATE_WASTED,
    DECODED_IMAGE_STATE_USED,
    DECODED_IMAGE_STATE_USED_RELOCK_FAILED,
    DECODED_IMAGE_STATE_WASTED_RELOCKED,
    DECODED_IMAGE_STATE_USED_RELOCKED,
    DECODED_IMAGE_STATE_COUNT
  } state = DECODED_IMAGE_STATE_WASTED;

  if (usage_stats_.lock_count == 1) {
    if (!usage_stats_.used)
      state = DECODED_IMAGE_STATE_WASTED;
    else if (usage_stats_.last_lock_failed)
      state = DECODED_IMAGE_STATE_USED_RELOCK_FAILED;
    else
      state = DECODED_IMAGE_STATE_USED;
  } else {
    if (usage_stats_.used)
      state = DECODED_IMAGE_STATE_USED_RELOCKED;
    else
      state = DECODED_IMAGE_STATE_WASTED_RELOCKED;
  }

  UMA_HISTOGRAM_ENUMERATION("Renderer4.SoftwareImageDecodeState", state,
                            DECODED_IMAGE_STATE_COUNT);
  UMA_HISTOGRAM_BOOLEAN("Renderer4.SoftwareImageDecodeState.FirstLockWasted",
                        usage_stats_.first_lock_wasted);
  if (usage_stats_.first_lock_out_of_raster)
    UMA_HISTOGRAM_BOOLEAN(
        "Renderer4.SoftwareImageDecodeState.FirstLockWasted.OutOfRaster",
        usage_stats_.first_lock_wasted);
}

void SoftwareImageDecodeCacheUtils::CacheEntry::TakeMemoryFromEntry(
    CacheEntry* entry) {
  DCHECK(!entry->is_budgeted_);
  DCHECK_EQ(entry->ref_count_, 0);

  // We explicitly don't copy ref counts or budgeted state.
  decode_failed_ = entry->decode_failed_;
  is_locked_ = entry->is_locked_;
  entry->is_locked_ = false;

  memory_ = std::move(entry->memory_);
  src_rect_offset_ = entry->src_rect_offset_;
  image_ = std::move(entry->image_);
}

bool SoftwareImageDecodeCacheUtils::CacheEntry::Lock() {
  if (!memory_)
    return false;

  DCHECK(!is_locked_);
  if (memory_->Lock()) {
    is_locked_ = true;
    ++usage_stats_.lock_count;
    return true;
  }
  ResetMemory();
  usage_stats_.last_lock_failed = true;
  return false;
}

void SoftwareImageDecodeCacheUtils::CacheEntry::Unlock() {
  if (!memory_)
    return;

  DCHECK(is_locked_);
  memory_->Unlock();
  is_locked_ = false;
  if (usage_stats_.lock_count == 1)
    usage_stats_.first_lock_wasted = !usage_stats_.used;
}

void SoftwareImageDecodeCacheUtils::CacheEntry::ResetMemory() {
  memory_ = nullptr;
  image_ = nullptr;
}

// Utils -----------------------------------------------------------------------
std::unique_ptr<SoftwareImageDecodeCacheUtils::CacheEntry>
SoftwareImageDecodeCacheUtils::Decode(const CacheKey& key,
                                      const PaintImage& paint_image,
                                      SkColorType color_type) {
  TRACE_EVENT2("cc", "SoftwareImageDecodeCacheUtils::Decode", "key",
               key.ToString(), "type",
               key.type() == CacheKey::kOriginal ? "decode original"
                                                 : "decode to scale");
  auto target_size = gfx::SizeToSkISize(key.target_size());
  // We can handle any type of processing here, given that we don't subrect and
  // that we can decode directly to the needed size.
  DCHECK(key.src_rect() == gfx::Rect(paint_image.width(), paint_image.height()))
      << key.src_rect().ToString() << " vs "
      << gfx::Rect(gfx::Rect(paint_image.width(), paint_image.height()))
             .ToString();

  DCHECK(target_size == paint_image.GetSupportedDecodeSize(target_size));

  SkImageInfo target_info =
      SkImageInfo::Make(target_size.width(), target_size.height(), color_type,
                        kPremul_SkAlphaType);
  std::unique_ptr<base::DiscardableMemory> target_pixels =
      base::DiscardableMemoryAllocator::GetInstance()
          ->AllocateLockedDiscardableMemory(target_info.minRowBytes() *
                                            target_info.height());
  DCHECK(target_pixels);

  bool result = paint_image.Decode(target_pixels->data(), &target_info,
                                   key.target_color_space().ToSkColorSpace(),
                                   key.frame_key().frame_index());
  if (!result) {
    target_pixels->Unlock();
    return nullptr;
  }

  return std::make_unique<CacheEntry>(target_info, std::move(target_pixels),
                                      SkSize::Make(0, 0));
}

std::unique_ptr<SoftwareImageDecodeCacheUtils::CacheEntry>
SoftwareImageDecodeCacheUtils::ScaleCandidateEntry(const CacheKey& key,
                                                   const CacheEntry* candidate,
                                                   bool needs_extract_subset,
                                                   SkColorType color_type) {
  TRACE_EVENT0("cc", "SoftwareImageDecodeCacheUtils::ScaleCandidateEntry");
  DCHECK(candidate);
  DCHECK(candidate->is_locked());
  DCHECK(candidate->image());

  SkISize target_size =
      SkISize::Make(key.target_size().width(), key.target_size().height());
  SkImageInfo target_info =
      SkImageInfo::Make(target_size.width(), target_size.height(), color_type,
                        kPremul_SkAlphaType);
  std::unique_ptr<base::DiscardableMemory> target_pixels =
      base::DiscardableMemoryAllocator::GetInstance()
          ->AllocateLockedDiscardableMemory(target_info.minRowBytes() *
                                            target_info.height());
  DCHECK(target_pixels);

  if (key.type() == CacheKey::kSubrectOriginal) {
    DCHECK(needs_extract_subset);
    TRACE_EVENT0(
        TRACE_DISABLED_BY_DEFAULT("cc.debug"),
        "SoftwareImageDecodeCacheUtils::ScaleCandidateEntry - subrect");
    bool result = candidate->image()->readPixels(
        target_info, target_pixels->data(), target_info.minRowBytes(),
        key.src_rect().x(), key.src_rect().y(), SkImage::kDisallow_CachingHint);
    // We have a decoded image, and we're reading into already allocated memory.
    // This should never fail.
    DCHECK(result) << key.ToString();
    return std::make_unique<CacheEntry>(
        target_info.makeColorSpace(candidate->image()->refColorSpace()),
        std::move(target_pixels),
        SkSize::Make(-key.src_rect().x(), -key.src_rect().y()));
  }

  DCHECK_EQ(key.type(), CacheKey::kSubrectAndScale);
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCacheUtils::ScaleCandidateEntry - scale");
  SkPixmap decoded_pixmap;
  bool result = candidate->image()->peekPixels(&decoded_pixmap);
  DCHECK(result) << key.ToString();
  if (needs_extract_subset) {
    result = decoded_pixmap.extractSubset(&decoded_pixmap,
                                          gfx::RectToSkIRect(key.src_rect()));
    DCHECK(result) << key.ToString();
  }

  // Nearest neighbor would only be set in the unscaled case.
  DCHECK(!key.is_nearest_neighbor());
  SkPixmap target_pixmap(target_info, target_pixels->data(),
                         target_info.minRowBytes());
  // Always use medium quality for scaling.
  result = decoded_pixmap.scalePixels(target_pixmap, kMedium_SkFilterQuality);
  DCHECK(result) << key.ToString();
  return std::make_unique<CacheEntry>(
      target_info.makeColorSpace(candidate->image()->refColorSpace()),
      std::move(target_pixels),
      SkSize::Make(-key.src_rect().x(), -key.src_rect().y()));
}

}  // namespace cc
