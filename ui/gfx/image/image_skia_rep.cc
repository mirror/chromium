// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/image/image_skia_rep.h"

#include "base/logging.h"
#include "cc/paint/paint_flags.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"

namespace gfx {

namespace {

class GFX_EXPORT BitmapDrawable : public DrawableSource {
 public:
  ~BitmapDrawable() override {}

  // Note: This is for testing purpose only.
  BitmapDrawable(const gfx::Size& size, float scale) : DrawableSource(size) {
    bitmap_.allocN32Pixels(static_cast<int>(size.width() * scale),
                           static_cast<int>(size.height() * scale));
    bitmap_.eraseColor(SK_ColorRED);
    bitmap_.setImmutable();
  }

  // Creates a bitmap with given scale.
  // Adds ref to |src|.
  BitmapDrawable(const SkBitmap& src)
      : DrawableSource(gfx::Size(src.width(), src.height())), bitmap_(src) {
    bitmap_.setImmutable();
  }

  void Draw(Canvas* canvas) const override {}

  const SkBitmap* GetBitmap() const override { return &bitmap_; }

 private:
  SkBitmap bitmap_;
};

}  // namespace

ImageSkiaRep::ImageSkiaRep() : scale_(0.0f) {
}

ImageSkiaRep::ImageSkiaRep(const ImageSkiaRep& other) = default;

ImageSkiaRep::~ImageSkiaRep() {
}

ImageSkiaRep::ImageSkiaRep(const gfx::Size& size, float scale) : scale_(scale) {
  source_.reset(new BitmapDrawable(size, scale));
}

ImageSkiaRep::ImageSkiaRep(const SkBitmap& src, float scale) : scale_(scale) {
  source_.reset(new BitmapDrawable(src));
}

int ImageSkiaRep::GetWidth() const {
  return static_cast<int>(source_->referenceSize().width() / scale());
}

int ImageSkiaRep::GetHeight() const {
  return static_cast<int>(source_->referenceSize().height() / scale());
}

void ImageSkiaRep::SetScaled() {
  DCHECK_EQ(0.0f, scale_);
  if (scale_ == 0.0f)
    scale_ = 1.0f;
}

const SkBitmap& ImageSkiaRep::sk_bitmap() const {
  if (!source_) {
    static SkBitmap null_bitmap;
    return null_bitmap;
  }
  if (source_->GetBitmap())
    return *source_->GetBitmap();
  if (bake_cache_.isNull()) {
    // TODO: bake here
  }
  return bake_cache_;
}

void ImageSkiaRep::Draw(Canvas* canvas, const cc::PaintFlags& flags) const {
  if (source_->GetBitmap()) {
    ScopedCanvas scoped(canvas);
    canvas->Scale(SkFloatToScalar(1.0f / scale_),
                  SkFloatToScalar(1.0f / scale_));
    canvas->DrawBitmap(*source_->GetBitmap(), flags);
  } else {
    source_->Draw(canvas);
  }
}

}  // namespace gfx
