// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/drawable/drawable_operations.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skbitmap_operations.h"

namespace gfx {

class TransparentDrawable : public DrawableSource {
 public:
  TransparentDrawable(const Drawable& base, float alpha)
      : DrawableSource(base.Size()), base_(base), alpha_(alpha) {}

  // DrawableSource implementation
  void Draw(Canvas* canvas) const override {
    canvas->SaveLayerAlpha(alpha_);
    base_.Draw(canvas);
    canvas->Restore();
  }

 private:
  Drawable base_;
  float alpha_;
};

Drawable DrawableOperations::CreateTransparentDrawable(const Drawable& base,
                                                       float alpha) {
  return Drawable(std::make_unique<TransparentDrawable>(base, alpha));
}

class ResizedDrawable : public DrawableSource {
 public:
  ResizedDrawable(const Drawable& base,
                  const gfx::Size& size,
                  skia::ImageOperations::ResizeMethod raster_resize_method)
      : DrawableSource(size),
        base_(base),
        raster_resize_method_(raster_resize_method) {}

  // DrawableSource implementation
  void Draw(Canvas* canvas) const override {
    canvas->Save();
    if ((raster_resize_method_ != skia::ImageOperations::RESIZE_GOOD)) {
      // TODO: need to somehow transfer resize mode or create a bitmap here.
      LOG(ERROR) << "UH OH " << raster_resize_method_;
    }
    canvas->Scale(size_.width() / base_.Size().width(),
                  size_.height() / base_.Size().height());
    base_.Draw(canvas);
    canvas->Restore();
  }

 private:
  Drawable base_;
  skia::ImageOperations::ResizeMethod raster_resize_method_;
};

Drawable DrawableOperations::CreateResizedDrawable(
    const Drawable& base,
    const gfx::Size& size,
    skia::ImageOperations::ResizeMethod raster_resize_method) {
  return Drawable(
      std::make_unique<ResizedDrawable>(base, size, raster_resize_method));
}

class BlendedDrawable : public DrawableSource {
 public:
  BlendedDrawable(const Drawable& first, const Drawable& second, float alpha)
      : DrawableSource(first.Size()),
        first_(first),
        second_(second),
        alpha_(alpha) {
    DCHECK(first_.Size() == second_.Size());
  }

  // DrawableSource implementation
  void Draw(Canvas* canvas) const override {
    canvas->SaveLayerAlpha(1.f);
    canvas->SaveLayerAlpha(1.f - alpha_);
    first_.Draw(canvas);
    canvas->Restore();
    canvas->SaveLayerAlpha(alpha_);
    second_.Draw(canvas);
    canvas->Restore();
    canvas->Restore();
  }

 private:
  Drawable first_;
  Drawable second_;
  float alpha_;
};

Drawable DrawableOperations::CreateBlendedDrawable(const Drawable& first,
                                                   const Drawable& second,
                                                   float alpha) {
  return Drawable(std::make_unique<BlendedDrawable>(first, second, alpha));
}

class SuperimposedDrawable : public DrawableSource {
 public:
  SuperimposedDrawable(const Drawable& bottom, const Drawable& top)
      : DrawableSource(bottom.Size()), bottom_(bottom), top_(top) {}

  // DrawableSource implementation
  void Draw(Canvas* canvas) const override {
    bottom_.Draw(canvas);
    top_.Draw(canvas, (bottom_.Size().width() - top_.Size().width()) / 2,
              (bottom_.Size().height() - top_.Size().height()) / 2);
  }

 private:
  Drawable bottom_;
  Drawable top_;
};

Drawable DrawableOperations::Superimpose(const Drawable& bottom,
                                         const Drawable& top) {
  return Drawable(std::make_unique<SuperimposedDrawable>(bottom, top));
}

// RotatedSource generates image reps that are rotations of those in
// |source| that represent requested scale factors.
class RotateDrawable : public DrawableSource {
 public:
  RotateDrawable(const Drawable& source,
                 DrawableOperations::RotationAmount rotation)
      : DrawableSource(
            rotation == DrawableOperations::ROTATION_180_CW
                ? source.Size()
                : gfx::Size(source.Size().height(), source.Size().width())),
        source_(source),
        rotation_(rotation) {}

  double GetDegrees() const {
    switch (rotation_) {
      case DrawableOperations::ROTATION_90_CW:
        return 90.0;
      case DrawableOperations::ROTATION_180_CW:
        return 180.0;
      case DrawableOperations::ROTATION_270_CW:
        return 270.0;
      default:
        DCHECK(false);  // Invalid Rotation.
    }
    return 0;
  }

  // gfx::ImageSkiaSource overrides:
  void Draw(Canvas* canvas) const override {
    canvas->Save();
    gfx::Transform xform;
    xform.Rotate(GetDegrees());
    canvas->Transform(xform);
    source_.Draw(canvas);
    canvas->Restore();
  }

 private:
  const Drawable source_;
  const DrawableOperations::RotationAmount rotation_;
};

Drawable DrawableOperations::CreateRotatedDrawable(
    const Drawable& source,
    DrawableOperations::RotationAmount rotation) {
  return Drawable(std::make_unique<RotateDrawable>(source, rotation));
}

// RotatedSource generates image reps that are rotations of those in
// |source| that represent requested scale factors.
class ColorDrawable : public DrawableSource {
 public:
  ColorDrawable(SkColor color, const gfx::Size& size)
      : DrawableSource(size), color_(color) {}

  // gfx::ImageSkiaSource overrides:
  void Draw(Canvas* canvas) const override {
    canvas->FillRect(gfx::Rect(0, 0, size_.width(), size_.height()), color_);
  }

 private:
  SkColor color_;
};

Drawable DrawableOperations::CreateSolidDrawable(SkColor color,
                                                 const gfx::Size& size) {
  return Drawable(std::make_unique<ColorDrawable>(color, size));
}

Drawable DrawableOperations::CreateEmptyDrawable() {
  // use defaults (which is bright pink)
  return CreateSolidDrawable();
}

}  // namespace gfx
