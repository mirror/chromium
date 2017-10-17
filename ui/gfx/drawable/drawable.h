// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_DRAWABLE_DRAWABLE_H_
#define UI_GFX_DRAWABLE_DRAWABLE_H_

#include <memory>

#include "ui/gfx/geometry/size.h"

class SkBitmap;

namespace gfx {
class Rect;

class Canvas;

class DrawableSource;
class Image;
class ImageSkia;

class GFX_EXPORT Drawable {
 public:
  Drawable();
  Drawable(const Drawable& other);

  template <class T>
  Drawable(
      std::unique_ptr<T>&& source,
      typename std::enable_if<
          std::is_convertible<T*, DrawableSource*>::value>::type* = nullptr)
      : source_(static_cast<DrawableSource*>(source.release())) {}
  ~Drawable();

  Drawable(const Image& image);
  Drawable(const ImageSkia& image_skia);
  Drawable(const ImageSkia* image_skia);

  operator Image() const;
  operator ImageSkia() const;

  void Draw(Canvas* canvas) const;
  void Draw(Canvas* canvas, int x, int y) const;
  void Draw(Canvas* canvas, const gfx::Rect& rect) const;
  gfx::Size Size() const;
  gfx::Size size() const;
  int width() const;
  int height() const;
  int Width() const;
  int Height() const;

  bool isNull() const { return !source_.get(); }

  DrawableSource* source() const { return source_.get(); }

  SkBitmap CreateBitmap(float scale) const;

  bool IsImageSkia() const;
  bool IsImage() const;
  const ImageSkia* ToImageSkia() const;
  const Image* ToImage() const;

  bool BackedBySameObjectAs(const Drawable& other) const;

  // will share backing storage if this drawable is already an imageskia
  ImageSkia MakeImageSkia() const;
  Image MakeImage() const;

 private:
  template <class T>
  void Init(std::unique_ptr<T>&& source) {
    source_.reset(static_cast<DrawableSource*>(source.release()));
  }

  std::shared_ptr<DrawableSource> source_;
};

class GFX_EXPORT DrawableSource {
 public:
  DrawableSource(const gfx::Size& size = gfx::Size());
  virtual ~DrawableSource();

  virtual void Draw(Canvas* canvas) const = 0;

  const gfx::Size& referenceSize() const { return size_; }

  virtual bool IsImageSkia() const;
  virtual bool IsImage() const;
  virtual const ImageSkia* ToImageSkia() const;
  virtual const Image* ToImage() const;

 protected:
  gfx::Size size_;
};

}  // namespace gfx

#endif  // UI_GFX_DRAWABLE_DRAWABLE_H_
