// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/drawable/drawable.h"

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skbitmap_operations.h"

namespace gfx {

namespace {

class DrawableImageSource : public ImageSkiaSource {
 public:
  DrawableImageSource(const Drawable& drawable) : drawable_(drawable) {}

  gfx::ImageSkiaRep GetImageForScale(float scale) override {
    gfx::Canvas canvas(drawable_.Size(), scale, false);
    drawable_.Draw(&canvas);
    return ImageSkiaRep(canvas.GetBitmap(), scale);
  }

  bool HasRepresentationAtAllScales() const override { return true; }

  gfx::Drawable* GetUnderlyingDrawable() const override {
    return const_cast<gfx::Drawable*>(&drawable_);
  }

 private:
  Drawable drawable_;
};

class GFX_EXPORT ImageSkiaDrawable : public DrawableSource {
 public:
  ImageSkiaDrawable(const ImageSkia& image_skia) : image_(image_skia) {
    size_ = image_skia.size();
  }

  ~ImageSkiaDrawable() override {}

  void Draw(Canvas* canvas) const override {
    canvas->DrawImageInt(image_, 0, 0);
  }

  bool IsImageSkia() const override { return true; }
  const ImageSkia* ToImageSkia() const override { return &image_; }

 private:
  ImageSkia image_;
};

class GFX_EXPORT ImageDrawable : public DrawableSource {
 public:
  ImageDrawable(const Image& image) : image_(image) { size_ = image.Size(); }

  ~ImageDrawable() override {}

  void Draw(Canvas* canvas) const override {
    canvas->DrawImageInt(*image_.ToImageSkia(), 0, 0);
  }

  bool IsImageSkia() const override {
    return image_.HasRepresentation(gfx::Image::kImageRepSkia);
  }
  bool IsImage() const override { return true; }
  const ImageSkia* ToImageSkia() const override {
    DCHECK(IsImageSkia());
    return image_.ToImageSkia();
  }
  const Image* ToImage() const override { return &image_; }

 private:
  Image image_;
};

}  // namespace

Drawable::Drawable() {}

Drawable::Drawable(const Drawable& other) : source_(other.source_) {}

Drawable::Drawable(const Image& image) {
  if (image.IsEmpty())
    return;
  if (image.HasRepresentation(gfx::Image::kImageRepSkia)) {
    Drawable* underlying = image.ToImageSkia()->GetUnderlyingDrawable();
    if (underlying) {
      source_ = underlying->source_;
      return;
    }
  }
  Init(std::make_unique<ImageDrawable>(image));
}

Drawable::Drawable(const ImageSkia& image_skia) {
  if (image_skia.isNull())
    return;
  Drawable* underlying = image_skia.GetUnderlyingDrawable();
  if (underlying)
    source_ = underlying->source_;
  else
    Init(std::make_unique<ImageSkiaDrawable>(image_skia));
}

Drawable::Drawable(const ImageSkia* image_skia) {
  if (!image_skia || image_skia->isNull())
    return;
  Drawable* underlying = image_skia->GetUnderlyingDrawable();
  if (underlying)
    source_ = underlying->source_;
  else
    Init(std::make_unique<ImageSkiaDrawable>(*image_skia));
}

Drawable::operator Image() const {
  return MakeImage();
}

Drawable::operator ImageSkia() const {
  return MakeImageSkia();
}

Drawable::~Drawable() {}

void Drawable::Draw(Canvas* canvas) const {
  DCHECK(source_.get());
  source_->Draw(canvas);
}

void Drawable::Draw(Canvas* canvas, int x, int y) const {
  DCHECK(source_.get());
  canvas->Save();
  canvas->Translate(gfx::Vector2d(x, y));
  source_->Draw(canvas);
  canvas->Restore();
}

void Drawable::Draw(Canvas* canvas, const gfx::Rect& dest) const {
  DCHECK(source_.get());
  canvas->Save();
  //    canvas->ClipRect(dest);
  canvas->Translate(gfx::Vector2d(dest.x(), dest.y()));
  gfx::Size ref_size = Size();
  if (dest.size() != ref_size) {
    canvas->Scale(static_cast<float>(dest.width()) / ref_size.width(),
                  static_cast<float>(dest.height()) / ref_size.height());
  }
  source_->Draw(canvas);
  canvas->Restore();
}

gfx::Size Drawable::Size() const {
  return isNull() ? gfx::Size() : source_->referenceSize();
}

gfx::Size Drawable::size() const {
  return Size();
}
int Drawable::width() const {
  return Size().width();
}
int Drawable::height() const {
  return Size().height();
}
int Drawable::Width() const {
  return Size().width();
}
int Drawable::Height() const {
  return Size().height();
}

bool Drawable::IsImage() const {
  return source_ && source_->IsImage();
}

bool Drawable::IsImageSkia() const {
  return source_ && source_->IsImageSkia();
}

const ImageSkia* Drawable::ToImageSkia() const {
  DCHECK(IsImageSkia());
  return source_->ToImageSkia();
}

const Image* Drawable::ToImage() const {
  DCHECK(IsImage());
  return source_->ToImage();
}

bool Drawable::BackedBySameObjectAs(const Drawable& other) const {
  if (IsImageSkia() && other.IsImageSkia())
    return ToImageSkia()->BackedBySameObjectAs(*other.ToImageSkia());
  else
    return source_.get() == other.source_.get();
}

ImageSkia Drawable::MakeImageSkia() const {
  if (isNull())
    return ImageSkia();
  if (IsImageSkia())
    return *ToImageSkia();
  else
    return ImageSkia(std::make_unique<DrawableImageSource>(*this), Size());
}

Image Drawable::MakeImage() const {
  if (isNull())
    return Image();
  if (IsImage())
    return *ToImage();
  else
    return Image(MakeImageSkia());
}

SkBitmap Drawable::CreateBitmap(float scale) const {
  Canvas canvas(Size(), scale, false);
  Draw(&canvas);
  return canvas.GetBitmap();
}

DrawableSource::DrawableSource(const gfx::Size& size) : size_(size) {}

DrawableSource::~DrawableSource() {}

bool DrawableSource::IsImageSkia() const {
  return false;
}

bool DrawableSource::IsImage() const {
  return false;
}
const ImageSkia* DrawableSource::ToImageSkia() const {
  return nullptr;
}
const Image* DrawableSource::ToImage() const {
  return nullptr;
}

}  // namespace gfx
