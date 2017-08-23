// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image.h"

#include "base/atomic_sequence_num.h"
#include "base/memory/ptr_util.h"
#include "cc/paint/paint_image_generator.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/skia_paint_image_generator.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "ui/gfx/skia_util.h"

namespace cc {
namespace {
base::AtomicSequenceNumber s_next_id_;
}

PaintImage::PaintImage() = default;
PaintImage::PaintImage(const PaintImage& other) = default;
PaintImage::PaintImage(PaintImage&& other) = default;
PaintImage::~PaintImage() = default;

PaintImage& PaintImage::operator=(const PaintImage& other) = default;
PaintImage& PaintImage::operator=(PaintImage&& other) = default;

bool PaintImage::operator==(const PaintImage& other) const {
  return id_ == other.id_ && sk_image_ == other.sk_image_ &&
         animation_type_ == other.animation_type_ &&
         completion_state_ == other.completion_state_ &&
         frame_count_ == other.frame_count_ &&
         is_multipart_ == other.is_multipart_;
}

PaintImage::Id PaintImage::GetNextId() {
  return s_next_id_.GetNext();
}

const sk_sp<SkImage>& PaintImage::GetSkImage() const {
  if (cached_sk_image_)
    return cached_sk_image_;

  if (sk_image_) {
    cached_sk_image_ = sk_image_;
  } else if (paint_record_) {
    cached_sk_image_ = SkImage::MakeFromPicture(
        ToSkPicture(paint_record_, gfx::RectToSkRect(paint_record_rect_)),
        SkISize::Make(paint_record_rect_.width(), paint_record_rect_.height()),
        nullptr, nullptr, SkImage::BitDepth::kU8, SkColorSpace::MakeSRGB());
  } else if (paint_image_generator_) {
    cached_sk_image_ = SkImage::MakeFromGenerator(
        base::MakeUnique<SkiaPaintImageGenerator>(paint_image_generator_));
  }
  return cached_sk_image_;
}

gfx::Size PaintImage::GetSupportedDecodeSize(
    const gfx::Size& requested_size) const {
  // TODO(vmpstr): For now, we ignore the requested size and just return the
  // available image size.
  return gfx::Size(width(), height());
}

size_t PaintImage::GetRequiredDecodeSizeBytes(
    const gfx::Size& size,
    viz::ResourceFormat format) const {
  DCHECK(GetSupportedDecodeSize(size) == size);
  auto info = SkImageInfo::Make(size.width(), size.height(),
                                viz::ResourceFormatToClosestSkColorType(format),
                                kPremul_SkAlphaType);
  return info.minRowBytes() * info.height();
}

bool PaintImage::Decode(void* memory,
                        const gfx::Size& size,
                        viz::ResourceFormat format,
                        const base::Optional<gfx::ColorSpace>& color_space,
                        SkImageInfo* decoded_info) const {
  SkImageInfo local_info;
  SkImageInfo* info = decoded_info ? decoded_info : &local_info;
  *info = SkImageInfo::Make(size.width(), size.height(),
                            viz::ResourceFormatToClosestSkColorType(format),
                            kPremul_SkAlphaType);
  auto image = GetSkImage();
  DCHECK(image);
  sk_sp<SkColorSpace> target_color_space;
  if (color_space)
    target_color_space = color_space->ToSkColorSpace();

  if (target_color_space) {
    image = image->makeColorSpace(target_color_space,
                                  SkTransferFunctionBehavior::kIgnore);
    if (!image)
      return false;
    *info = info->makeColorSpace(target_color_space);
  }
  return image->readPixels(*info, memory, info->minRowBytes(), 0, 0,
                           SkImage::kDisallow_CachingHint);
}

}  // namespace cc
