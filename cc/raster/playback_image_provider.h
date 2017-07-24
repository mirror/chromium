// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RASTER_PLAYBACK_IMAGE_PROVIDER_H_
#define CC_RASTER_PLAYBACK_IMAGE_PROVIDER_H_

#include "cc/cc_export.h"
#include "cc/paint/image_id.h"
#include "cc/paint/image_provider.h"
#include "ui/gfx/color_space.h"

namespace cc {
class ImageDecodeCache;

class CC_EXPORT PlaybackImageProvider : public ImageProvider {
 public:
  PlaybackImageProvider(bool skip_all_images,
                        PaintImageIdFlatSet images_to_skip,
                        ImageDecodeCache* cache,
                        const gfx::ColorSpace& taget_color_space);
  ~PlaybackImageProvider() override;

  // ImageProvider implementation.
  std::unique_ptr<DecodedImageHolder> GetDecodedImage(
      const PaintImage& paint_image,
      const SkRect& src_rect,
      SkFilterQuality filter_quality,
      const SkMatrix& matrix) override;

 private:
  const bool skip_all_images_;
  const PaintImageIdFlatSet images_to_skip_;
  ImageDecodeCache* cache_;
  const gfx::ColorSpace target_color_space_;
};

}  // namespace cc

#endif  // CC_RASTER_PLAYBACK_IMAGE_PROVIDER_H_
