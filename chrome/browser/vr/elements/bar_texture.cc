// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/bar_texture.h"

#include "ui/gfx/canvas.h"

namespace vr {

BarTexture::BarTexture() = default;

BarTexture::~BarTexture() = default;

void BarTexture::Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) {
  pixel_size_.set_width(texture_size.width());
  pixel_size_.set_height(texture_size.height());

  SkPaint paint;
  float radius = pixel_size_.width() * 0.5f;
  paint.setColor(color_);
  sk_canvas->drawRoundRect(
      SkRect::MakeXYWH(0, 0, pixel_size_.width(), pixel_size_.height()), radius,
      radius, paint);
}

gfx::Size BarTexture::GetPreferredTextureSize(int maximum_width) const {
  return gfx::Size(maximum_width,
                   maximum_width * size_.height() / size_.width());
}

gfx::SizeF BarTexture::GetDrawnSize() const {
  return pixel_size_;
}

void BarTexture::SetSize(float width, float height) {
  size_.set_width(width);
  size_.set_height(height);
  set_dirty();
}

void BarTexture::SetColor(SkColor color) {
  color_ = color;
  set_dirty();
}

}  // namespace vr
