// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_PAINT_IMAGE_BUILDER_H_
#define CC_PAINT_PAINT_IMAGE_BUILDER_H_

#include "base/memory/ptr_util.h"
#include "cc/paint/paint_export.h"
#include "cc/paint/paint_image.h"
#include "cc/paint/paint_image_generator.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/skia_paint_image_generator.h"
#include "third_party/skia/include/core/SkImage.h"

namespace cc {

class CC_PAINT_EXPORT PaintImageBuilder {
 public:
  PaintImageBuilder();
  ~PaintImageBuilder();

  void set_id(PaintImage::Id id) { paint_image_.id_ = id; }
  void set_image(sk_sp<SkImage> sk_image) {
    paint_image_.sk_image_ = std::move(sk_image);
  }
  void set_paint_record(sk_sp<PaintRecord> paint_record,
                        const gfx::Rect& rect) {
    paint_image_.paint_record_ = std::move(paint_record);
    paint_image_.paint_record_rect_ = rect;
  }
  void set_animation_type(PaintImage::AnimationType type) {
    paint_image_.animation_type_ = type;
  }
  void set_completion_state(PaintImage::CompletionState state) {
    paint_image_.completion_state_ = state;
  }
  void set_frame_count(size_t frame_count) {
    paint_image_.frame_count_ = frame_count;
  }
  void set_is_multipart(bool is_multipart) {
    paint_image_.is_multipart_ = is_multipart;
  }

  void set_paint_image_generator(sk_sp<PaintImageGenerator> generator) {
    paint_image_.paint_image_generator_ = generator;
  }

  uint32_t set_sk_image_id(uint32_t unique_id) {
    DCHECK(paint_image_.paint_image_generator_);

    paint_image_.cached_sk_image_ =
        SkImage::MakeFromGenerator(base::MakeUnique<SkiaPaintImageGenerator>(
            paint_image_.paint_image_generator_, unique_id));
    return paint_image_.cached_sk_image_->uniqueID();
  }

  PaintImage TakePaintImage() const;

 private:
  PaintImage paint_image_;
};

}  // namespace cc

#endif  // CC_PAINT_PAINT_IMAGE_BUILDER_H_
