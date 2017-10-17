// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_DRAWABLE_DRAWABLE_OPERATIONS_H_
#define UI_GFX_DRAWABLE_DRAWABLE_OPERATIONS_H_

#include <memory>

#include "skia/ext/image_operations.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/drawable/drawable.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

namespace gfx {

class GFX_EXPORT DrawableOperations {
 public:
  // Enum for use in rotating images (must be in 90 degree increments),
  // see: CreateRotatedDrawable.
  enum RotationAmount {
    ROTATION_90_CW,
    ROTATION_180_CW,
    ROTATION_270_CW,
  };

  static Drawable CreateTransparentDrawable(const Drawable& base, float alpha);
  static Drawable CreateResizedDrawable(
      const Drawable& base,
      const gfx::Size& size,
      skia::ImageOperations::ResizeMethod raster_resize_method =
          skia::ImageOperations::RESIZE_GOOD);
  static Drawable CreateBlendedDrawable(const Drawable& first,
                                        const Drawable& second,
                                        float alpha);
  static Drawable Superimpose(const Drawable& bottom, const Drawable& top);
  static Drawable CreateRotatedDrawable(const Drawable& base,
                                        RotationAmount rotation);
  static Drawable CreateSolidDrawable(
      SkColor color = SkColorSetARGB(255, 255, 255, 0),
      const gfx::Size& size = gfx::Size(1, 1));
  static Drawable CreateEmptyDrawable();
};

}  // namespace gfx

#endif  // UI_GFX_DRAWABLE_DRAWABLE_OPERATIONS_H_
