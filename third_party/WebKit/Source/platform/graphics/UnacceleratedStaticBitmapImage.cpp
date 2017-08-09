// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/UnacceleratedStaticBitmapImage.h"

#include "third_party/skia/include/core/SkImage.h"

namespace blink {

PassRefPtr<UnacceleratedStaticBitmapImage>
UnacceleratedStaticBitmapImage::Create(sk_sp<SkImage> image) {
  DCHECK(!image->isTextureBacked());
  return AdoptRef(new UnacceleratedStaticBitmapImage(std::move(image)));
}

UnacceleratedStaticBitmapImage::UnacceleratedStaticBitmapImage(
    sk_sp<SkImage> image)
    : image_(std::move(image)) {
  DCHECK(image_);
  DCHECK(!image_->isLazyGenerated());
}

PassRefPtr<UnacceleratedStaticBitmapImage>
UnacceleratedStaticBitmapImage::Create(PaintImage image) {
  return AdoptRef(new UnacceleratedStaticBitmapImage(std::move(image)));
}

UnacceleratedStaticBitmapImage::UnacceleratedStaticBitmapImage(PaintImage image)
    : paint_image_(std::move(image)) {
  DCHECK(paint_image_);
}

UnacceleratedStaticBitmapImage::~UnacceleratedStaticBitmapImage() {}

IntSize UnacceleratedStaticBitmapImage::Size() const {
  return IntSize(GetSkImage()->width(), GetSkImage()->height());
}

bool UnacceleratedStaticBitmapImage::CurrentFrameKnownToBeOpaque(MetadataMode) {
  return GetSkImage()->isOpaque();
}

void UnacceleratedStaticBitmapImage::Draw(PaintCanvas* canvas,
                                          const PaintFlags& flags,
                                          const FloatRect& dst_rect,
                                          const FloatRect& src_rect,
                                          RespectImageOrientationEnum,
                                          ImageClampingMode clamp_mode) {
  StaticBitmapImage::DrawHelper(canvas, flags, dst_rect, src_rect, clamp_mode,
                                PaintImageForCurrentFrame());
}

void UnacceleratedStaticBitmapImage::PopulateImageForCurrentFrame(
    PaintImageBuilder& builder) {
  if (paint_image_) {
    builder.set_paint_image(paint_image_);
    builder.set_is_static(true);
  } else {
    builder.set_image(image_);
  }
}

const sk_sp<SkImage>& UnacceleratedStaticBitmapImage::GetSkImage() const {
  if (paint_image_) {
    DCHECK(!image_);
    return paint_image_.GetSkImage();
  }
  return image_;
}

}  // namespace blink
