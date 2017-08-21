// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/splash_screen_text_texture.h"

#include "ui/gfx/canvas.h"

namespace vr {

SplashScreenTextTexture::SplashScreenTextTexture() = default;

SplashScreenTextTexture::~SplashScreenTextTexture() = default;

void SplashScreenTextTexture::SetSplashScreenTextBitmap(
    const SkBitmap& bitmap) {
  splash_screen_text_ = SkImage::MakeFromBitmap(bitmap);
  set_dirty();
}

void SplashScreenTextTexture::Draw(SkCanvas* sk_canvas,
                                   const gfx::Size& texture_size) {
  size_.set_width(texture_size.width());
  size_.set_height(texture_size.height());
  sk_canvas->drawImageRect(
      splash_screen_text_,
      SkRect::MakeXYWH(0, 0, size_.width(), size_.height()), nullptr);
}

gfx::Size SplashScreenTextTexture::GetPreferredTextureSize(
    int maximum_width) const {
  return gfx::Size(maximum_width, maximum_width);
}

gfx::SizeF SplashScreenTextTexture::GetDrawnSize() const {
  return size_;
}

}  // namespace vr
