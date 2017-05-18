// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_DISPLAY_ITEM_LIST_H_
#define CC_PAINT_DISPLAY_ITEM_LIST_H_

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/rtree.h"
#include "cc/paint/discardable_image_map.h"
#include "cc/paint/image_id.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/paint_op_buffer.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"

class SkCanvas;

namespace base {
namespace trace_event {
class TracedValue;
}
}

namespace cc {
class DisplayItem;

class CC_PAINT_EXPORT DisplayItemList
    : public base::RefCountedThreadSafe<DisplayItemList> {
 public:
  DisplayItemList();

  void Raster(SkCanvas* canvas,
              SkPicture::AbortCallback* callback = nullptr) const;

  PaintOpBuffer* StartPaint() {
    DCHECK(!current_range_start_);
    current_range_start_ = paint_op_buffer_.size();
    return &paint_op_buffer_;
  }

  void EndPaintOfUnpaired(const gfx::Rect& visual_rect) {
    if (paint_op_buffer_.size() == current_range_start_)
      return;
    visual_rects_.push_back(visual_rect);
    visual_rects_range_starts_.push_back(current_range_start_);
    GrowCurrentBeginItemVisualRect(visual_rect);

    current_range_start_ = 0;
  }

  void EndPaintOfPairedBegin(const gfx::Rect& visual_rect = gfx::Rect()) {
    DCHECK_NE(current_range_start_, paint_op_buffer_.size());
    size_t visual_rect_index = visual_rects_.size();
    visual_rects_.push_back(visual_rect);
    visual_rects_range_starts_.push_back(current_range_start_);
    begin_paired_indices_.push_back(visual_rect_index);

    current_range_start_ = 0;
  }

  void EndPaintOfPairedEnd() {
    DCHECK_NE(current_range_start_, paint_op_buffer_.size());

    // Copy the visual rect of the matching kPairStart.
    size_t last_begin_index = begin_paired_indices_.back();
    begin_paired_indices_.pop_back();
    visual_rects_.push_back(visual_rects_[last_begin_index]);
    visual_rects_range_starts_.push_back(current_range_start_);

    // The block that ended needs to be included in the bounds of the enclosing
    // block.
    GrowCurrentBeginItemVisualRect(visual_rects_[last_begin_index]);

    current_range_start_ = 0;
  }

  // Called after all items are appended, to process the items.
  void Finalize();

  void SetIsSuitableForGpuRasterization(bool is_suitable) {
    all_items_are_suitable_for_gpu_rasterization_ = is_suitable;
  }
  bool IsSuitableForGpuRasterization() const;

  size_t OpCount() const;
  size_t ApproximateMemoryUsage() const;
  bool ShouldBeAnalyzedForSolidColor() const;

  void EmitTraceSnapshot() const;

  void GenerateDiscardableImagesMetadata();
  void GetDiscardableImagesInRect(const gfx::Rect& rect,
                                  float contents_scale,
                                  const gfx::ColorSpace& target_color_space,
                                  std::vector<DrawImage>* images);
  gfx::Rect GetRectForImage(PaintImage::Id image_id) const;

  void SetRetainVisualRectsForTesting(bool retain) {
    retain_visual_rects_ = retain;
  }

  // XXX remove one of these two.
  size_t size() const { return OpCount(); }

  gfx::Rect VisualRectForTesting(int index) { return visual_rects_[index]; }

  void GatherDiscardableImages(DiscardableImageStore* image_store) const;
  const DiscardableImageMap& discardable_image_map_for_testing() const {
    return image_map_;
  }

  bool HasDiscardableImages() const {
    return paint_op_buffer_.HasDiscardableImages();
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(DisplayItemListTest, AsValueWithNoItems);
  FRIEND_TEST_ALL_PREFIXES(DisplayItemListTest, AsValueWithItems);

  ~DisplayItemList();

  std::unique_ptr<base::trace_event::TracedValue> CreateTracedValue(
      bool include_items) const;

  // If we're currently within a paired display item block, unions the
  // given visual rect with the begin display item's visual rect.
  void GrowCurrentBeginItemVisualRect(const gfx::Rect& visual_rect);

  RTree rtree_;
  DiscardableImageMap image_map_;
  PaintOpBuffer paint_op_buffer_;

  // The visual rects associated with each of the display items in the
  // display item list. There is one rect per range in
  // visual_rects_range_starts_. These rects are intentionally kept separate
  // because they are used to decide which ops to walk for raster.
  std::vector<gfx::Rect> visual_rects_;
  // For each Rect in visual_rects_, this is the start of the range of
  // PaintOps in the PaintOpBuffer that the Rect describes. The range ends
  // at the start of the next index in the array.
  std::vector<size_t> visual_rects_range_starts_;
  // A stack of indices into the |visual_rects_| for each paired begin range
  // that hasn't been closed.
  std::vector<size_t> begin_paired_indices_;
  // While recording a range of ops, this is the position in the PaintOpBuffer
  // where the recording started.
  size_t current_range_start_ = 0;

  bool all_items_are_suitable_for_gpu_rasterization_ = true;
  // For testing purposes only. Whether to keep visual rects across calls to
  // Finalize().
  bool retain_visual_rects_ = false;

  friend class base::RefCountedThreadSafe<DisplayItemList>;
  FRIEND_TEST_ALL_PREFIXES(DisplayItemListTest, ApproximateMemoryUsage);
  DISALLOW_COPY_AND_ASSIGN(DisplayItemList);
};

}  // namespace cc

#endif  // CC_PAINT_DISPLAY_ITEM_LIST_H_
