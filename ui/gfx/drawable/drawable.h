// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_DRAWABLE_DRAWABLE_H_
#define UI_GFX_DRAWABLE_DRAWABLE_H_

#include <memory>

#include "ui/gfx/geometry/size.h"

class SkBitmap;

namespace gfx {

class Canvas;

class GFX_EXPORT DrawableSource {
 public:
  DrawableSource(const gfx::Size& size = gfx::Size());
  virtual ~DrawableSource();

  virtual void Draw(Canvas* canvas) const = 0;

  const gfx::Size& referenceSize() const { return size_; }

  virtual const SkBitmap* GetBitmap() const;

 protected:
  gfx::Size size_;
};

}  // namespace gfx

#endif  // UI_GFX_DRAWABLE_DRAWABLE_H_
