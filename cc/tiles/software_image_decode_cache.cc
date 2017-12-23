// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/software_image_decode_cache.h"

#include <inttypes.h>
#include <stdint.h>

#include <algorithm>
#include <functional>
#include <tuple>

#include "base/format_macros.h"
#include "base/macros.h"
#include "base/memory/memory_coordinator_client_registry.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "cc/base/devtools_instrumentation.h"
#include "cc/base/histograms.h"
#include "cc/raster/tile_task.h"
#include "cc/tiles/mipmap_util.h"
#include "cc/tiles/software_image_decode_cache_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "ui/gfx/skia_util.h"

using base::trace_event::MemoryAllocatorDump;
using base::trace_event::MemoryDumpLevelOfDetail;

namespace cc {
namespace {

// The number of entries to keep around in the cache. This limit can be breached
// if more items are locked. That is, locked items ignore this limit.
// Depending on the memory state of the system, we limit the amount of items
// differently.
const size_t kNormalMaxItemsInCacheForSoftware = 1000;
const size_t kThrottledMaxItemsInCacheForSoftware = 100;
const size_t kSuspendedMaxItemsInCacheForSoftware = 0;

class AutoRemoveKeyFromTaskMap {
 public:
  AutoRemoveKeyFromTaskMap(
      std::unordered_map<SoftwareImageDecodeCache::CacheKey,
                         scoped_refptr<TileTask>,
                         SoftwareImageDecodeCache::CacheKeyHash>* task_map,
      const SoftwareImageDecodeCache::CacheKey& key)
      : task_map_(task_map), key_(key) {}
  ~AutoRemoveKeyFromTaskMap() { task_map_->erase(key_); }

 private:
  std::unordered_map<SoftwareImageDecodeCache::CacheKey,
                     scoped_refptr<TileTask>,
                     SoftwareImageDecodeCache::CacheKeyHash>* task_map_;
  const SoftwareImageDecodeCache::CacheKey& key_;
};

class SoftwareImageDecodeTaskImpl : public TileTask {
 public:
  SoftwareImageDecodeTaskImpl(
      SoftwareImageDecodeCache* cache,
      const SoftwareImageDecodeCache::CacheKey& image_key,
      const PaintImage& paint_image,
      SoftwareImageDecodeCache::DecodeTaskType task_type,
      const ImageDecodeCache::TracingInfo& tracing_info)
      : TileTask(true),
        cache_(cache),
        image_key_(image_key),
        paint_image_(paint_image),
        task_type_(task_type),
        tracing_info_(tracing_info) {}

  // Overridden from Task:
  void RunOnWorkerThread() override {
    TRACE_EVENT2("cc", "SoftwareImageDecodeTaskImpl::RunOnWorkerThread", "mode",
                 "software", "source_prepare_tiles_id",
                 tracing_info_.prepare_tiles_id);
    devtools_instrumentation::ScopedImageDecodeTask image_decode_task(
        paint_image_.GetSkImage().get(),
        devtools_instrumentation::ScopedImageDecodeTask::kSoftware,
        ImageDecodeCache::ToScopedTaskType(tracing_info_.task_type));
    cache_->DecodeImageInTask(image_key_, paint_image_, task_type_);
  }

  // Overridden from TileTask:
  void OnTaskCompleted() override {
    cache_->OnImageDecodeTaskCompleted(image_key_, task_type_);
  }

 protected:
  ~SoftwareImageDecodeTaskImpl() override = default;

 private:
  SoftwareImageDecodeCache* cache_;
  SoftwareImageDecodeCache::CacheKey image_key_;
  PaintImage paint_image_;
  SoftwareImageDecodeCache::DecodeTaskType task_type_;
  const ImageDecodeCache::TracingInfo tracing_info_;

  DISALLOW_COPY_AND_ASSIGN(SoftwareImageDecodeTaskImpl);
};

SkSize GetScaleAdjustment(const SoftwareImageDecodeCache::CacheKey& key) {
  // If the requested filter quality did not require scale, then the adjustment
  // is identity.
  if (key.type() != SoftwareImageDecodeCache::CacheKey::kSubrectAndScale) {
    return SkSize::Make(1.f, 1.f);
  } else {
    return MipMapUtil::GetScaleAdjustmentForSize(key.src_rect().size(),
                                                 key.target_size());
  }
}

// Returns the filter quality to be used with the decoded result of the image.
// Note that in most cases this yields Low filter quality, meaning bilinear
// interpolation. This is because the processing for the image would have
// already been done, including scaling down to a mip level. So what remains is
// to do a bilinear interpolation. The exception to this is if the developer
// specified a pixelated effect, which results in a None filter quality (nearest
// neighbor).
SkFilterQuality GetDecodedFilterQuality(
    const SoftwareImageDecodeCache::CacheKey& key) {
  return key.is_nearest_neighbor() ? kNone_SkFilterQuality
                                   : kLow_SkFilterQuality;
}

void RecordLockExistingCachedImageHistogram(TilePriority::PriorityBin bin,
                                            bool success) {
  switch (bin) {
    case TilePriority::NOW:
      UMA_HISTOGRAM_BOOLEAN("Renderer4.LockExistingCachedImage.Software.NOW",
                            success);
    case TilePriority::SOON:
      UMA_HISTOGRAM_BOOLEAN("Renderer4.LockExistingCachedImage.Software.SOON",
                            success);
    case TilePriority::EVENTUALLY:
      UMA_HISTOGRAM_BOOLEAN(
          "Renderer4.LockExistingCachedImage.Software.EVENTUALLY", success);
  }
}

}  // namespace

SoftwareImageDecodeCache::SoftwareImageDecodeCache(
    SkColorType color_type,
    size_t locked_memory_limit_bytes)
    : decoded_images_(ImageMRUCache::NO_AUTO_EVICT),
      locked_images_budget_(locked_memory_limit_bytes),
      color_type_(color_type),
      max_items_in_cache_(kNormalMaxItemsInCacheForSoftware) {
  // In certain cases, ThreadTaskRunnerHandle isn't set (Android Webview).
  // Don't register a dump provider in these cases.
  if (base::ThreadTaskRunnerHandle::IsSet()) {
    base::trace_event::MemoryDumpManager::GetInstance()->RegisterDumpProvider(
        this, "cc::SoftwareImageDecodeCache",
        base::ThreadTaskRunnerHandle::Get());
  }
  // Register this component with base::MemoryCoordinatorClientRegistry.
  base::MemoryCoordinatorClientRegistry::GetInstance()->Register(this);
}

SoftwareImageDecodeCache::~SoftwareImageDecodeCache() {
  // It is safe to unregister, even if we didn't register in the constructor.
  base::trace_event::MemoryDumpManager::GetInstance()->UnregisterDumpProvider(
      this);
  // Unregister this component with memory_coordinator::ClientRegistry.
  base::MemoryCoordinatorClientRegistry::GetInstance()->Unregister(this);

  // TODO(vmpstr): If we don't have a client name, it may cause problems in
  // unittests, since most tests don't set the name but some do. The UMA system
  // expects the name to be always the same. This assertion is violated in the
  // tests that do set the name.
  if (GetClientNameForMetrics()) {
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        base::StringPrintf("Compositing.%s.CachedImagesCount.Software",
                           GetClientNameForMetrics()),
        lifetime_max_items_in_cache_, 1, 1000, 20);
  }
}

ImageDecodeCache::TaskResult SoftwareImageDecodeCache::GetTaskForImageAndRef(
    const DrawImage& image,
    const TracingInfo& tracing_info) {
  DCHECK_EQ(tracing_info.task_type, TaskType::kInRaster);
  return GetTaskForImageAndRefInternal(image, tracing_info,
                                       DecodeTaskType::USE_IN_RASTER_TASKS);
}

ImageDecodeCache::TaskResult
SoftwareImageDecodeCache::GetOutOfRasterDecodeTaskForImageAndRef(
    const DrawImage& image) {
  return GetTaskForImageAndRefInternal(
      image, TracingInfo(0, TilePriority::NOW, TaskType::kOutOfRaster),
      DecodeTaskType::USE_OUT_OF_RASTER_TASKS);
}

ImageDecodeCache::TaskResult
SoftwareImageDecodeCache::GetTaskForImageAndRefInternal(
    const DrawImage& image,
    const TracingInfo& tracing_info,
    DecodeTaskType task_type) {
  CacheKey key = CacheKey::FromDrawImage(image, color_type_);
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::GetTaskForImageAndRefInternal", "key",
               key.ToString());

  // If the target size is empty, we can skip this image during draw (and thus
  // we don't need to decode it or ref it).
  if (key.target_size().IsEmpty())
    return TaskResult(false);

  base::AutoLock lock(lock_);

  bool new_image_fits_in_memory =
      locked_images_budget_.AvailableMemoryBytes() >= key.locked_bytes();

  // Get or generate the cache entry.
  auto decoded_it = decoded_images_.Get(key);
  CacheEntry* cache_entry = nullptr;
  if (decoded_it == decoded_images_.end()) {
    // There is no reason to create a new entry if we know it won't fit anyway.
    if (!new_image_fits_in_memory)
      return TaskResult(false);
    cache_entry = AddCacheEntry(key);
    if (task_type == DecodeTaskType::USE_OUT_OF_RASTER_TASKS)
      cache_entry->set_out_of_raster();
  } else {
    cache_entry = decoded_it->second.get();
  }
  DCHECK(cache_entry);

  if (!cache_entry->is_budgeted()) {
    if (!new_image_fits_in_memory) {
      // We don't need to ref anything here because this image will be at
      // raster.
      return TaskResult(false);
    }
    AddBudgetForImage(key, cache_entry);
  }
  DCHECK(cache_entry->is_budgeted());

  // The rest of the code will return either true or a task, so we should ref
  // the image once now for the caller to unref.
  cache_entry->ref();

  // If we already have a locked entry, then we can just use that. Otherwise
  // we'll have to create a task.
  if (cache_entry->is_locked())
    return TaskResult(true);

  auto& task =
      cache_entry->task(task_type == DecodeTaskType::USE_IN_RASTER_TASKS
                            ? CacheEntry::TaskType::kInRaster
                            : CacheEntry::TaskType::kOutOfRaster);
  if (!task) {
    // Ref image once for the decode task.
    cache_entry->ref();
    task = base::MakeRefCounted<SoftwareImageDecodeTaskImpl>(
        this, key, image.paint_image(), task_type, tracing_info);
  }
  return TaskResult(task);
}

void SoftwareImageDecodeCache::AddBudgetForImage(const CacheKey& key,
                                                 CacheEntry* entry) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::AddBudgetForImage", "key",
               key.ToString());
  lock_.AssertAcquired();

  DCHECK(!entry->is_budgeted());
  DCHECK_GE(locked_images_budget_.AvailableMemoryBytes(), key.locked_bytes());
  locked_images_budget_.AddUsage(key.locked_bytes());
  entry->set_budgeted(true);
}

void SoftwareImageDecodeCache::RemoveBudgetForImage(const CacheKey& key,
                                                    CacheEntry* entry) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::RemoveBudgetForImage", "key",
               key.ToString());
  lock_.AssertAcquired();

  DCHECK(entry->is_budgeted());
  locked_images_budget_.SubtractUsage(key.locked_bytes());
  entry->set_budgeted(false);
}

void SoftwareImageDecodeCache::UnrefImage(const DrawImage& image) {
  const CacheKey& key = CacheKey::FromDrawImage(image, color_type_);
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::UnrefImage", "key", key.ToString());

  base::AutoLock lock(lock_);
  UnrefImage(key);
}

void SoftwareImageDecodeCache::UnrefImage(const CacheKey& key) {
  lock_.AssertAcquired();
  auto decoded_image_it = decoded_images_.Peek(key);
  DCHECK(decoded_image_it != decoded_images_.end());
  auto* entry = decoded_image_it->second.get();
  DCHECK(entry->has_refs());
  if (entry->unref() == 0) {
    if (entry->is_budgeted())
      RemoveBudgetForImage(key, entry);
    if (entry->is_locked())
      entry->Unlock();
  }
}

void SoftwareImageDecodeCache::DecodeImageInTask(const CacheKey& key,
                                                 const PaintImage& paint_image,
                                                 DecodeTaskType task_type) {
  TRACE_EVENT1("cc", "SoftwareImageDecodeCache::DecodeImageInTask", "key",
               key.ToString());
  base::AutoLock lock(lock_);

  auto image_it = decoded_images_.Peek(key);
  DCHECK(image_it != decoded_images_.end());
  auto* cache_entry = image_it->second.get();
  // These two checks must be true because we're running this from a task, which
  // means that we've budgeted this entry when we got the task and the ref count
  // is also held by the task (released in OnTaskCompleted).
  DCHECK(cache_entry->has_refs());
  DCHECK(cache_entry->is_budgeted());

  DecodeImageIfNecessary(key, paint_image, cache_entry);
  DCHECK(cache_entry->decode_failed() || cache_entry->is_locked());
  RecordImageMipLevelUMA(
      MipMapUtil::GetLevelForSize(key.src_rect().size(), key.target_size()));
}

void SoftwareImageDecodeCache::DecodeImageIfNecessary(const CacheKey& key,
                                                      const PaintImage& image,
                                                      CacheEntry* cache_entry) {
  DCHECK(cache_entry->has_refs());
  if (!NeedsDecode(key, cache_entry))
    return;

  auto decoded_entry = DecodeImage(key, image);
  if (!decoded_entry) {
    cache_entry->set_decode_failed();
    return;
  }

  DCHECK(decoded_entry->is_locked());
  // We could've decoded this entry when we released the lock in
  // DecodeImage() on a different thread.
  if (cache_entry->is_locked()) {
    // Unlock our work.
    decoded_entry->Unlock();
    return;
  }
  // Update the cache entry with our memory.
  cache_entry->TakeMemoryFromEntry(decoded_entry.get());
}

bool SoftwareImageDecodeCache::NeedsDecode(const CacheKey& key,
                                           CacheEntry* entry) const {
  if (key.target_size().IsEmpty())
    entry->set_decode_failed();

  if (entry->decode_failed())
    return false;

  if (entry->image()) {
    if (entry->is_locked())
      return false;

    bool lock_succeeded = entry->Lock();
    // TODO(vmpstr): Deprecate the prepaint split, since it doesn't matter.
    RecordLockExistingCachedImageHistogram(TilePriority::NOW, lock_succeeded);

    if (lock_succeeded)
      return false;
  }
  return true;
}

std::unique_ptr<SoftwareImageDecodeCache::CacheEntry>
SoftwareImageDecodeCache::DecodeImage(const CacheKey& key,
                                      const PaintImage& paint_image) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::DecodeImage", "key", key.ToString());
  lock_.AssertAcquired();

  // If we need the original decode, just decode and return it. Easy.
  if (key.type() == CacheKey::kOriginal) {
    base::AutoUnlock release(lock_);
    return Utils::Decode(key, paint_image, color_type_);
  }

  // See if we can find a candidate to scale.
  CacheEntry* candidate_entry = nullptr;
  auto candidate_key = FindCandidateKeyAndLock(key, &candidate_entry);
  if (candidate_key) {
    if (candidate_entry->decode_failed())
      return nullptr;
    DCHECK(candidate_entry->is_locked());
    DCHECK(candidate_entry->image());
    // We locked the entry as a part of finding the key. If we're the only lock
    // holder then we should unlock it afterwards, however, if we're not the
    // only one then we have to keep the image locked. The way to solve this is
    // to simply ref the candidate and unref it after we're done. The reffing
    // system will take care of any unlocks that may be required.
    candidate_entry->ref();

    // Scale to produce an entry. Note that the only time we need to subrect the
    // candidate is if it was an original decode.
    std::unique_ptr<CacheEntry> entry;
    {
      base::AutoUnlock release(lock_);
      entry = Utils::ScaleCandidateEntry(
          key, candidate_entry, candidate_key->type() == CacheKey::kOriginal,
          color_type_);
      DCHECK(entry);
    }

    // Balance the ref in this block.
    UnrefImage(*candidate_key);
    return entry;
  }

  // See if we can decode to scale.
  SkISize target_size = gfx::SizeToSkISize(key.target_size());
  gfx::Rect full_rect = gfx::Rect(paint_image.width(), paint_image.height());
  if (key.src_rect() == full_rect &&
      paint_image.GetSupportedDecodeSize(target_size) == target_size) {
    // If we have a full rect src, and we're not in the kOriginal case, then we
    // must be in the scale case.
    DCHECK_EQ(key.type(), CacheKey::kSubrectAndScale);

    base::AutoUnlock release(lock_);
    return Utils::Decode(key, paint_image, color_type_);
  }

  // In all other cases, we need the original decode first.
  auto src_rect = gfx::RectToSkIRect(full_rect);
  DrawImage candidate_draw_image(paint_image, src_rect, kNone_SkFilterQuality,
                                 SkMatrix::I(), key.frame_key().frame_index(),
                                 key.target_color_space());
  auto original_key =
      CacheKey::FromDrawImage(candidate_draw_image, color_type_);
  DCHECK_EQ(original_key.type(), CacheKey::kOriginal);

  std::unique_ptr<CacheEntry> original_entry;
  {
    base::AutoUnlock release(lock_);
    original_entry = Utils::Decode(original_key, paint_image, color_type_);
  }

  // Couldn't do a decode.
  if (!original_entry)
    return nullptr;

  // It is important for us to cache this original entry to prevent constantly
  // decoding the original if we need other scales later. First, see if we
  // already have the original entry, which would be the case if some other
  // thread decoded it already.
  candidate_entry = nullptr;
  auto decoded_it = decoded_images_.Get(original_key);
  if (decoded_it == decoded_images_.end()) {
    // No entry exists, so cache it.
    candidate_entry = AddCacheEntry(original_key);
    candidate_entry->TakeMemoryFromEntry(original_entry.get());
  } else {
    // Entry exists, see if we can lock it.
    candidate_entry = decoded_it->second.get();
    if (candidate_entry->is_locked() || candidate_entry->Lock()) {
      // We locked the candidate, so we have to unlock our (wasted) work.
      original_entry->Unlock();
    } else {
      // Candidate existed, but we couldn't lock it, update its memory with our
      // decode.
      candidate_entry->TakeMemoryFromEntry(original_entry.get());
    }
  }
  DCHECK(!original_entry->is_locked());
  DCHECK(candidate_entry->is_locked());
  // We may or may not have used an existing entry, but in either case we can
  // delete the original decode since we would've cached it if it wasn't cached
  // before.
  original_entry.reset();

  // Do a similar trick with ref/unref so that any possible unlocking happens
  // automatically.
  candidate_entry->set_used();
  candidate_entry->ref();
  std::unique_ptr<CacheEntry> entry;
  {
    base::AutoUnlock release(lock_);
    entry = Utils::ScaleCandidateEntry(key, candidate_entry, true /* subrect */,
                                       color_type_);
  }
  UnrefImage(original_key);
  return entry;
}

base::Optional<SoftwareImageDecodeCache::CacheKey>
SoftwareImageDecodeCache::FindCandidateKeyAndLock(const CacheKey& key,
                                                  CacheEntry** entry) {
  auto image_keys_it = frame_key_to_image_keys_.find(key.frame_key());
  // We know that we must have at least the entry that wanted to find the
  // candidate in this list, so it won't be empty.
  DCHECK(image_keys_it != frame_key_to_image_keys_.end());

  auto& available_keys = image_keys_it->second;
  std::sort(available_keys.begin(), available_keys.end(),
            [](const CacheKey& one, const CacheKey& two) {
              // Return true if |one| scale is less than |two| scale.
              return std::make_tuple(one.target_size().width(),
                                     one.target_size().height()) <
                     std::make_tuple(two.target_size().width(),
                                     two.target_size().height());
            });

  base::Optional<CacheKey> result;
  for (auto& available_key : available_keys) {
    // Only consider keys coming from the same src rect, since otherwise the
    // resulting image was extracted using a different src.
    if (available_key.src_rect() != key.src_rect())
      continue;

    // that are at least as big as the required |key|.
    if (available_key.target_size().width() < key.target_size().width() ||
        available_key.target_size().height() < key.target_size().height()) {
      continue;
    }
    auto image_it = decoded_images_.Peek(available_key);
    DCHECK(image_it != decoded_images_.end());
    auto* available_entry = image_it->second.get();
    if (available_entry->is_locked() || available_entry->Lock()) {
      *entry = available_entry;
      return available_key;
    }
  }
  *entry = nullptr;
  return {};
}

DecodedDrawImage SoftwareImageDecodeCache::GetDecodedImageForDraw(
    const DrawImage& draw_image) {
  base::AutoLock hold(lock_);
  return GetDecodedImageForDrawInternal(
      CacheKey::FromDrawImage(draw_image, color_type_),
      draw_image.paint_image());
}

DecodedDrawImage SoftwareImageDecodeCache::GetDecodedImageForDrawInternal(
    const CacheKey& key,
    const PaintImage& paint_image) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::GetDecodedImageForDrawInternal",
               "key", key.ToString());

  lock_.AssertAcquired();
  auto decoded_it = decoded_images_.Get(key);
  CacheEntry* cache_entry = nullptr;
  if (decoded_it == decoded_images_.end())
    cache_entry = AddCacheEntry(key);
  else
    cache_entry = decoded_it->second.get();

  // We'll definitely ref this cache entry and use it.
  cache_entry->ref();
  cache_entry->set_used();

  DecodeImageIfNecessary(key, paint_image, cache_entry);
  DCHECK(cache_entry->decode_failed() || cache_entry->is_locked());
  auto decoded_draw_image =
      DecodedDrawImage(cache_entry->image(), cache_entry->src_rect_offset(),
                       GetScaleAdjustment(key), GetDecodedFilterQuality(key));
  return decoded_draw_image;
}

void SoftwareImageDecodeCache::DrawWithImageFinished(
    const DrawImage& image,
    const DecodedDrawImage& decoded_image) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
               "SoftwareImageDecodeCache::DrawWithImageFinished", "key",
               CacheKey::FromDrawImage(image, color_type_).ToString());
  UnrefImage(image);
}

void SoftwareImageDecodeCache::ReduceCacheUsageUntilWithinLimit(size_t limit) {
  TRACE_EVENT0("cc",
               "SoftwareImageDecodeCache::ReduceCacheUsageUntilWithinLimit");
  lifetime_max_items_in_cache_ =
      std::max(lifetime_max_items_in_cache_, decoded_images_.size());
  for (auto it = decoded_images_.rbegin();
       decoded_images_.size() > limit && it != decoded_images_.rend();) {
    if (it->second->has_refs()) {
      ++it;
      continue;
    }

    const CacheKey& key = it->first;
    auto vector_it = frame_key_to_image_keys_.find(key.frame_key());
    auto item_it =
        std::find(vector_it->second.begin(), vector_it->second.end(), key);
    DCHECK(item_it != vector_it->second.end());
    vector_it->second.erase(item_it);
    if (vector_it->second.empty())
      frame_key_to_image_keys_.erase(vector_it);

    it = decoded_images_.Erase(it);
  }
}

void SoftwareImageDecodeCache::ReduceCacheUsage() {
  base::AutoLock lock(lock_);
  ReduceCacheUsageUntilWithinLimit(max_items_in_cache_);
}

void SoftwareImageDecodeCache::ClearCache() {
  base::AutoLock lock(lock_);
  ReduceCacheUsageUntilWithinLimit(0);
}

size_t SoftwareImageDecodeCache::GetMaximumMemoryLimitBytes() const {
  return locked_images_budget_.total_limit_bytes();
}

void SoftwareImageDecodeCache::NotifyImageUnused(
    const PaintImage::FrameKey& frame_key) {
  base::AutoLock lock(lock_);

  auto it = frame_key_to_image_keys_.find(frame_key);
  if (it == frame_key_to_image_keys_.end())
    return;

  for (auto key_it = it->second.begin(); key_it != it->second.end();) {
    // This iterates over the CacheKey vector for the given skimage_id,
    // and deletes all entries from decoded_images_ corresponding to the
    // skimage_id.
    auto image_it = decoded_images_.Peek(*key_it);
    // TODO(sohanjg): Find an optimized way to cleanup locked images.
    if (image_it != decoded_images_.end() && !image_it->second->has_refs()) {
      decoded_images_.Erase(image_it);
      key_it = it->second.erase(key_it);
    } else {
      ++key_it;
    }
  }
  if (it->second.empty())
    frame_key_to_image_keys_.erase(it);
}

void SoftwareImageDecodeCache::OnImageDecodeTaskCompleted(
    const CacheKey& key,
    DecodeTaskType task_type) {
  base::AutoLock hold(lock_);

  auto image_it = decoded_images_.Peek(key);
  DCHECK(image_it != decoded_images_.end());
  CacheEntry* cache_entry = image_it->second.get();
  auto& task =
      cache_entry->task(task_type == DecodeTaskType::USE_IN_RASTER_TASKS
                            ? CacheEntry::TaskType::kInRaster
                            : CacheEntry::TaskType::kOutOfRaster);
  task = nullptr;

  UnrefImage(key);
}

bool SoftwareImageDecodeCache::OnMemoryDump(
    const base::trace_event::MemoryDumpArgs& args,
    base::trace_event::ProcessMemoryDump* pmd) {
  base::AutoLock lock(lock_);

  if (args.level_of_detail == MemoryDumpLevelOfDetail::BACKGROUND) {
    std::string dump_name = base::StringPrintf(
        "cc/image_memory/cache_0x%" PRIXPTR, reinterpret_cast<uintptr_t>(this));
    MemoryAllocatorDump* dump = pmd->CreateAllocatorDump(dump_name);
    dump->AddScalar("locked_size", MemoryAllocatorDump::kUnitsBytes,
                    locked_images_budget_.GetCurrentUsageSafe());
  } else {
    for (const auto& image_pair : decoded_images_) {
      int image_id = static_cast<int>(image_pair.first.frame_key().hash());
      CacheEntry* entry = image_pair.second.get();
      DCHECK(entry);
      // We might not have memory for this cache entry, depending on where int
      // he CacheEntry lifecycle we are. If we don't have memory, then we don't
      // have to record it in the dump.
      if (!entry->memory_for_tracing())
        continue;

      std::string dump_name = base::StringPrintf(
          "cc/image_memory/cache_0x%" PRIXPTR "/%s/image_%" PRIu64 "_id_%d",
          reinterpret_cast<uintptr_t>(this),
          entry->is_budgeted() ? "budgeted" : "at_raster", entry->tracing_id(),
          image_id);
      // CreateMemoryAllocatorDump will automatically add tracking values for
      // the total size. We also add a "locked_size" below.
      MemoryAllocatorDump* dump =
          entry->memory_for_tracing()->CreateMemoryAllocatorDump(
              dump_name.c_str(), pmd);
      DCHECK(dump);
      size_t locked_bytes =
          entry->is_locked() ? image_pair.first.locked_bytes() : 0u;
      dump->AddScalar("locked_size", MemoryAllocatorDump::kUnitsBytes,
                      locked_bytes);
    }
  }

  // Memory dump can't fail, always return true.
  return true;
}

void SoftwareImageDecodeCache::OnMemoryStateChange(base::MemoryState state) {
  {
    base::AutoLock hold(lock_);
    switch (state) {
      case base::MemoryState::NORMAL:
        max_items_in_cache_ = kNormalMaxItemsInCacheForSoftware;
        break;
      case base::MemoryState::THROTTLED:
        max_items_in_cache_ = kThrottledMaxItemsInCacheForSoftware;
        break;
      case base::MemoryState::SUSPENDED:
        max_items_in_cache_ = kSuspendedMaxItemsInCacheForSoftware;
        break;
      case base::MemoryState::UNKNOWN:
        NOTREACHED();
        return;
    }
  }
}

void SoftwareImageDecodeCache::OnPurgeMemory() {
  base::AutoLock lock(lock_);
  ReduceCacheUsageUntilWithinLimit(0);
}

SoftwareImageDecodeCache::CacheEntry* SoftwareImageDecodeCache::AddCacheEntry(
    const CacheKey& key) {
  lock_.AssertAcquired();
  frame_key_to_image_keys_[key.frame_key()].push_back(key);
  auto it = decoded_images_.Put(key, std::make_unique<CacheEntry>());
  it->second.get()->set_cached();
  return it->second.get();
}

// MemoryBudget ----------------------------------------------------------------
SoftwareImageDecodeCache::MemoryBudget::MemoryBudget(size_t limit_bytes)
    : limit_bytes_(limit_bytes), current_usage_bytes_(0u) {}

size_t SoftwareImageDecodeCache::MemoryBudget::AvailableMemoryBytes() const {
  size_t usage = GetCurrentUsageSafe();
  return usage >= limit_bytes_ ? 0u : (limit_bytes_ - usage);
}

void SoftwareImageDecodeCache::MemoryBudget::AddUsage(size_t usage) {
  current_usage_bytes_ += usage;
}

void SoftwareImageDecodeCache::MemoryBudget::SubtractUsage(size_t usage) {
  DCHECK_GE(current_usage_bytes_.ValueOrDefault(0u), usage);
  current_usage_bytes_ -= usage;
}

void SoftwareImageDecodeCache::MemoryBudget::ResetUsage() {
  current_usage_bytes_ = 0;
}

size_t SoftwareImageDecodeCache::MemoryBudget::GetCurrentUsageSafe() const {
  return current_usage_bytes_.ValueOrDie();
}

}  // namespace cc
