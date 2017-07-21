// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/playback_image_provider.h"

#include "base/memory/ptr_util.h"
#include "cc/tiles/image_decode_cache.h"

namespace cc {
namespace {
SkIRect RoundOutRect(const SkRect& rect) {
  SkIRect result;
  rect.roundOut(&result);
  return result;
}

class DecodedImageHolderImpl : public ImageProvider::DecodedImageHolder {
 public:
  DecodedImageHolderImpl(ImageDecodeCache* cache,
                         PaintImage paint_image,
                         const SkRect& src_rect,
                         const SkMatrix& matrix,
                         SkFilterQuality quality,
                         const gfx::ColorSpace& target_color_space)
      : cache_(cache),
        draw_image_(std::move(paint_image),
                    RoundOutRect(src_rect),
                    quality,
                    matrix,
                    target_color_space),
        decoded_draw_image_(cache_->GetDecodedImageForDraw(draw_image_)) {}

  const DecodedDrawImage& DecodedImage() override {
    return decoded_draw_image_;
  }

  ~DecodedImageHolderImpl() override {
    cache_->DrawWithImageFinished(draw_image_, decoded_draw_image_);
  }

 private:
  ImageDecodeCache* cache_;
  DrawImage draw_image_;
  DecodedDrawImage decoded_draw_image_;
};

}  // namespace

PlaybackImageProvider::PlaybackImageProvider(
    PaintImageIdFlatSet images_to_skip,
    ImageDecodeCache* cache,
    const gfx::ColorSpace& target_color_space)
    : images_to_skip_(std::move(images_to_skip)),
      cache_(cache),
      target_color_space_(target_color_space) {
  DCHECK(cache_);
}

PlaybackImageProvider::~PlaybackImageProvider() = default;

std::unique_ptr<ImageProvider::DecodedImageHolder>
PlaybackImageProvider::GetDecodedImage(const PaintImage& paint_image,
                                       const SkRect& src_rect,
                                       SkFilterQuality filter_quality,
                                       const SkMatrix& matrix) {
  DCHECK(paint_image.sk_image()->isLazyGenerated());

  if (images_to_skip_.count(paint_image.stable_id()) != 0)
    return nullptr;

  return base::MakeUnique<DecodedImageHolderImpl>(cache_, paint_image, src_rect,
                                                  matrix, filter_quality,
                                                  target_color_space_);
}

}  // namespace cc
