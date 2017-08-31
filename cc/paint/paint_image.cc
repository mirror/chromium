// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image.h"

#include "base/atomic_sequence_num.h"
#include "base/memory/ptr_util.h"
#include "cc/paint/paint_image_generator.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/skia_paint_image_generator.h"
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
  return sk_image_ == other.sk_image_ && paint_record_ == other.paint_record_ &&
         paint_record_rect_ == other.paint_record_rect_ &&
         paint_image_generator_ == other.paint_image_generator_ &&
         id_ == other.id_ && animation_type_ == other.animation_type_ &&
         completion_state_ == other.completion_state_ &&
         subset_rect_ == other.subset_rect_ &&
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
        std::make_unique<SkiaPaintImageGenerator>(paint_image_generator_));
  }

  if (!subset_rect_.IsEmpty() && cached_sk_image_) {
    cached_sk_image_ =
        cached_sk_image_->makeSubset(gfx::RectToSkIRect(subset_rect_));
  }
  return cached_sk_image_;
}

PaintImage PaintImage::MakeSubset(const gfx::Rect& subset) const {
  DCHECK(!subset.IsEmpty());

  // If the subset is the same as the image bounds, we can return the same
  // image.
  gfx::Rect bounds(width(), height());
  if (bounds == subset)
    return *this;

  DCHECK(bounds.Contains(subset))
      << "Subset should not be greater than the image bounds";
  PaintImage result(*this);
  result.subset_rect_ = subset;
  // Store the subset from the original image.
  result.subset_rect_.Offset(subset_rect_.x(), subset_rect_.y());

  // Creating the |cached_sk_image_| is an optimization to allow re-use of the
  // original decode for image subsets in skia, for cases that rely on skia's
  // image decode cache.
  // TODO(khushalsagar): Remove this when we no longer have such cases. See
  // crbug.com/753639.
  result.cached_sk_image_ =
      GetSkImage()->makeSubset(gfx::RectToSkIRect(subset));
  return result;
}

SkISize PaintImage::GetSupportedDecodeSize(
    const SkISize& requested_size) const {
  // TODO(vmpstr): If this image is using subset_rect, then we don't support
  // decoding to any scale other than the original. See the comment in Decode()
  // explaining this in more detail.
  if (paint_image_generator_ && subset_rect_.IsEmpty())
    return paint_image_generator_->GetSupportedDecodeSize(requested_size);
  return SkISize::Make(width(), height());
}

SkImageInfo PaintImage::CreateDecodeImageInfo(const SkISize& size,
                                              SkColorType color_type) const {
  DCHECK(GetSupportedDecodeSize(size) == size);
  return SkImageInfo::Make(size.width(), size.height(), color_type,
                           kPremul_SkAlphaType);
}

bool PaintImage::Decode(void* memory,
                        SkImageInfo* info,
                        sk_sp<SkColorSpace> color_space) const {
  // We only support decode to supported decode size.
  DCHECK(info->dimensions() == GetSupportedDecodeSize(info->dimensions()));

  // TODO(vmpstr): If we're using a subset_rect_ then the info specifies the
  // requested size relative to the subset. However, the generator isn't aware
  // of this subsetting and would need a size that is relative to the original
  // image size. We could still implement this case, but we need to convert the
  // requested size into the space of the original image. For now, fallback to
  // DecodeFromSkImage().
  if (paint_image_generator_ && subset_rect_.IsEmpty())
    return DecodeFromGenerator(memory, info, std::move(color_space));
  return DecodeFromSkImage(memory, info, std::move(color_space));
}

bool PaintImage::DecodeFromGenerator(void* memory,
                                     SkImageInfo* info,
                                     sk_sp<SkColorSpace> color_space) const {
  DCHECK(subset_rect_.IsEmpty());

  // First convert the info to have the requested color space, since the decoder
  // will convert this for us.
  *info = info->makeColorSpace(std::move(color_space));
  if (info->colorType() != kN32_SkColorType) {
    // Since the decoders only support N32 color types, make one of those and
    // decode into temporary memory. Then read the bitmap which will convert it
    // to the target color type.
    SkImageInfo n32info = info->makeColorType(kN32_SkColorType);
    std::unique_ptr<char[]> n32memory(
        new char[n32info.minRowBytes() * n32info.height()]);

    bool result = paint_image_generator_->GetPixels(
        n32info, n32memory.get(), n32info.minRowBytes(), unique_id());
    if (!result)
      return false;

    SkBitmap bitmap;
    bitmap.installPixels(n32info, n32memory.get(), n32info.minRowBytes());
    return bitmap.readPixels(*info, memory, info->minRowBytes(), 0, 0);
  }

  return paint_image_generator_->GetPixels(*info, memory, info->minRowBytes(),
                                           unique_id());
}

bool PaintImage::DecodeFromSkImage(void* memory,
                                   SkImageInfo* info,
                                   sk_sp<SkColorSpace> color_space) const {
  auto image = GetSkImage();
  DCHECK(image);
  if (color_space) {
    image =
        image->makeColorSpace(color_space, SkTransferFunctionBehavior::kIgnore);
    if (!image)
      return false;
  }
  // Note that the readPixels has to happen before converting the info to the
  // given color space, since it can produce incorrect results.
  bool result = image->readPixels(*info, memory, info->minRowBytes(), 0, 0,
                                  SkImage::kDisallow_CachingHint);
  *info = info->makeColorSpace(std::move(color_space));
  return result;
}

}  // namespace cc
