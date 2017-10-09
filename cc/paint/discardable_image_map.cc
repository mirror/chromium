// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/discardable_image_map.h"

#include <stddef.h>

#include <algorithm>
#include <limits>

#include "base/containers/adapters.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/paint_op_buffer.h"
#include "third_party/skia/include/utils/SkNoDrawCanvas.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/skia_util.h"

namespace cc {
namespace {

class DiscardableImageGenerator {
 public:
  DiscardableImageGenerator(int width,
                            int height,
                            const PaintOpBuffer* buffer,
                            const std::vector<gfx::Rect>& visual_rects) {
    SkNoDrawCanvas canvas(width, height);
    GatherDiscardableImages(buffer, &visual_rects, nullptr, &canvas);
  }
  ~DiscardableImageGenerator() = default;

  std::vector<std::pair<DrawImage, gfx::Rect>> TakeImages() {
    return std::move(image_set_);
  }
  base::flat_map<PaintImage::Id, gfx::Rect> TakeImageIdToRectMap() {
    return std::move(image_id_to_rect_);
  }
  std::vector<DiscardableImageMap::AnimatedImageMetadata>
  TakeAnimatedImagesMetadata() {
    return std::move(animated_images_metadata_);
  }

  void RecordColorHistograms() const {
    if (color_stats_total_image_count_ > 0) {
      int srgb_image_percent = (100 * color_stats_srgb_image_count_) /
                               color_stats_total_image_count_;
      UMA_HISTOGRAM_PERCENTAGE("Renderer4.ImagesPercentSRGB",
                               srgb_image_percent);
    }

    base::CheckedNumeric<int> srgb_pixel_percent =
        100 * color_stats_srgb_pixel_count_ / color_stats_total_pixel_count_;
    if (srgb_pixel_percent.IsValid()) {
      UMA_HISTOGRAM_PERCENTAGE("Renderer4.ImagePixelsPercentSRGB",
                               srgb_pixel_percent.ValueOrDie());
    }
  }

  bool all_images_are_srgb() const {
    return color_stats_srgb_image_count_ == color_stats_total_image_count_;
  }

 private:
  // Adds discardable images from |buffer| to the set of images tracked by
  // this generator. If |buffer| is being used in a DrawOp that requires
  // rasterization of the buffer as a pre-processing step for execution of the
  // op (for instance, with PaintRecord backed PaintShaders),
  // |top_level_op_rect| is set to the rect for that op. If provided, the
  // |top_level_op_rect| will be used as the rect for tracking the position of
  // this image in the top-level buffer.
  void GatherDiscardableImages(const PaintOpBuffer* buffer,
                               const std::vector<gfx::Rect>* visual_rects,
                               const gfx::Rect* top_level_op_rect,
                               SkNoDrawCanvas* canvas) {
    DCHECK_NE(!visual_rects, !top_level_op_rect);
    if (!buffer->HasDiscardableImages())
      return;

    std::vector<gfx::Rect>::const_iterator rect_iter;
    if (visual_rects) {
      rect_iter = visual_rects->begin();
      DCHECK_EQ(visual_rects->size(), buffer->size());
    }

    // Prevent PaintOpBuffers from having side effects back into the canvas.
    SkAutoCanvasRestore save_restore(canvas, true);

    gfx::Rect op_rect;
    if (top_level_op_rect)
      op_rect = *top_level_op_rect;
    PlaybackParams params(nullptr, canvas->getTotalMatrix());
    // TODO(khushalsagar): Optimize out save/restore blocks if there are no
    // images in the draw ops between them.
    for (auto* op : PaintOpBuffer::Iterator(buffer)) {
      if (visual_rects) {
        op_rect = *rect_iter;
        ++rect_iter;
      }
      if (!op->IsDrawOp()) {
        op->Raster(canvas, params);
        continue;
      } else if (!PaintOp::OpHasDiscardableImages(op)) {
        continue;
      }

      const SkMatrix& ctm = canvas->getTotalMatrix();
      if (op->IsPaintOpWithFlags()) {
        AddImageFromFlags(op_rect,
                          static_cast<const PaintOpWithFlags*>(op)->flags, ctm);
      }

      PaintOpType op_type = static_cast<PaintOpType>(op->type);
      if (op_type == PaintOpType::DrawImage) {
        auto* image_op = static_cast<DrawImageOp*>(op);
        auto* sk_image = image_op->image.GetSkImage().get();
        AddImage(image_op->image,
                 SkRect::MakeIWH(sk_image->width(), sk_image->height()),
                 op_rect, ctm, image_op->flags.getFilterQuality());
      } else if (op_type == PaintOpType::DrawImageRect) {
        auto* image_rect_op = static_cast<DrawImageRectOp*>(op);
        SkMatrix matrix = ctm;
        matrix.postConcat(SkMatrix::MakeRectToRect(image_rect_op->src,
                                                   image_rect_op->dst,
                                                   SkMatrix::kFill_ScaleToFit));
        AddImage(image_rect_op->image, image_rect_op->src, op_rect, matrix,
                 image_rect_op->flags.getFilterQuality());
      } else if (op_type == PaintOpType::DrawRecord) {
        GatherDiscardableImages(
            static_cast<const DrawRecordOp*>(op)->record.get(), nullptr,
            &op_rect, canvas);
      }
    }
  }

  void AddImageFromFlags(const gfx::Rect& op_rect,
                         const PaintFlags& flags,
                         const SkMatrix& ctm) {
    if (!flags.getShader())
      return;

    if (flags.getShader()->shader_type() == PaintShader::Type::kImage) {
      const PaintImage& paint_image = flags.getShader()->paint_image();
      SkMatrix matrix = ctm;
      matrix.postConcat(flags.getShader()->GetLocalMatrix());
      AddImage(paint_image,
               SkRect::MakeWH(paint_image.width(), paint_image.height()),
               op_rect, matrix, flags.getFilterQuality());
    } else if (flags.getShader()->shader_type() ==
                   PaintShader::Type::kPaintRecord &&
               flags.getShader()->paint_record()->HasDiscardableImages()) {
      SkRect scaled_tile_rect;
      if (!flags.getShader()->GetRasterizationTileRect(ctm,
                                                       &scaled_tile_rect)) {
        return;
      }

      SkNoDrawCanvas canvas(scaled_tile_rect.width(),
                            scaled_tile_rect.height());
      canvas.setMatrix(SkMatrix::MakeRectToRect(flags.getShader()->tile(),
                                                scaled_tile_rect,
                                                SkMatrix::kFill_ScaleToFit));
      GatherDiscardableImages(flags.getShader()->paint_record().get(), nullptr,
                              &op_rect, &canvas);
    }
  }

  void AddImage(PaintImage paint_image,
                const SkRect& src_rect,
                const gfx::Rect& visual_rect,
                const SkMatrix& matrix,
                SkFilterQuality filter_quality) {
    if (!paint_image.IsLazyGenerated())
      return;

    SkIRect src_irect;
    src_rect.roundOut(&src_irect);

    // Make a note if any image was originally specified in a non-sRGB color
    // space.
    SkColorSpace* source_color_space = paint_image.color_space();
    color_stats_total_pixel_count_ += visual_rect.size().GetCheckedArea();
    color_stats_total_image_count_++;
    if (!source_color_space || source_color_space->isSRGB()) {
      color_stats_srgb_pixel_count_ += visual_rect.size().GetCheckedArea();
      color_stats_srgb_image_count_++;
    }

    // During raster, we use the device clip bounds on the canvas, which outsets
    // the actual clip by 1 due to the possibility of antialiasing. Account for
    // this here by outsetting the image rect by 1. Note that this only affects
    // queries into the rtree, which will now return images that only touch the
    // bounds of the query rect.
    //
    // Note that it's not sufficient for us to inset the device clip bounds at
    // raster time, since we might be sending a larger-than-one-item display
    // item to skia, which means that skia will internally determine whether to
    // raster the picture (using device clip bounds that are outset).
    gfx::Rect image_rect = visual_rect;
    image_rect.Inset(-1, -1);

    image_id_to_rect_[paint_image.stable_id()].Union(image_rect);

    if (paint_image.ShouldAnimate()) {
      animated_images_metadata_.emplace_back(
          paint_image.stable_id(), paint_image.completion_state(),
          paint_image.GetFrameMetadata(), paint_image.repetition_count(),
          paint_image.reset_animation_sequence_id());
    }

    image_set_.emplace_back(
        DrawImage(std::move(paint_image), src_irect, filter_quality, matrix),
        image_rect);
  }

  std::vector<std::pair<DrawImage, gfx::Rect>> image_set_;
  base::flat_map<PaintImage::Id, gfx::Rect> image_id_to_rect_;
  std::vector<DiscardableImageMap::AnimatedImageMetadata>
      animated_images_metadata_;

  // Statistics about the number of images and pixels that will require color
  // conversion if the target color space is not sRGB.
  int color_stats_srgb_image_count_ = 0;
  int color_stats_total_image_count_ = 0;
  base::CheckedNumeric<int64_t> color_stats_srgb_pixel_count_ = 0;
  base::CheckedNumeric<int64_t> color_stats_total_pixel_count_ = 0;
};

}  // namespace

DiscardableImageMap::DiscardableImageMap() = default;
DiscardableImageMap::~DiscardableImageMap() = default;

void DiscardableImageMap::Generate(const PaintOpBuffer* paint_op_buffer,
                                   const gfx::Rect& bounds,
                                   const std::vector<gfx::Rect>& visual_rects) {
  TRACE_EVENT0("cc", "DiscardableImageMap::Generate");

  if (!paint_op_buffer->HasDiscardableImages())
    return;

  DiscardableImageGenerator generator(bounds.right(), bounds.bottom(),
                                      paint_op_buffer, visual_rects);
  generator.RecordColorHistograms();
  image_id_to_rect_ = generator.TakeImageIdToRectMap();
  animated_images_metadata_ = generator.TakeAnimatedImagesMetadata();
  all_images_are_srgb_ = generator.all_images_are_srgb();
  auto images = generator.TakeImages();
  images_rtree_.Build(
      images,
      [](const std::vector<std::pair<DrawImage, gfx::Rect>>& items,
         size_t index) { return items[index].second; },
      [](const std::vector<std::pair<DrawImage, gfx::Rect>>& items,
         size_t index) { return items[index].first; });
}

void DiscardableImageMap::GetDiscardableImagesInRect(
    const gfx::Rect& rect,
    std::vector<const DrawImage*>* images) const {
  *images = images_rtree_.SearchRefs(rect);
}

gfx::Rect DiscardableImageMap::GetRectForImage(PaintImage::Id image_id) const {
  const auto& it = image_id_to_rect_.find(image_id);
  return it == image_id_to_rect_.end() ? gfx::Rect() : it->second;
}

void DiscardableImageMap::Reset() {
  image_id_to_rect_.clear();
  image_id_to_rect_.shrink_to_fit();
  images_rtree_.Reset();
}

DiscardableImageMap::AnimatedImageMetadata::AnimatedImageMetadata(
    PaintImage::Id paint_image_id,
    PaintImage::CompletionState completion_state,
    std::vector<FrameMetadata> frames,
    int repetition_count,
    PaintImage::AnimationSequenceId reset_animation_sequence_id)
    : paint_image_id(paint_image_id),
      completion_state(completion_state),
      frames(std::move(frames)),
      repetition_count(repetition_count),
      reset_animation_sequence_id(reset_animation_sequence_id) {}

DiscardableImageMap::AnimatedImageMetadata::~AnimatedImageMetadata() = default;

DiscardableImageMap::AnimatedImageMetadata::AnimatedImageMetadata(
    const AnimatedImageMetadata& other) = default;

}  // namespace cc
