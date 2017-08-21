// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_SPLASH_SCREEN_ICON_TEXTURE_H_
#define CHROME_BROWSER_VR_ELEMENTS_SPLASH_SCREEN_ICON_TEXTURE_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "third_party/skia/include/core/SkImage.h"

namespace vr {

class SplashScreenTextTexture : public UiTexture {
 public:
  SplashScreenTextTexture();
  ~SplashScreenTextTexture() override;
  gfx::Size GetPreferredTextureSize(int width) const override;
  gfx::SizeF GetDrawnSize() const override;

  void SetSplashScreenTextBitmap(const SkBitmap& bitmap);

 private:
  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  sk_sp<SkImage> splash_screen_text_;
  gfx::SizeF size_;

  DISALLOW_COPY_AND_ASSIGN(SplashScreenTextTexture);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_SPLASH_SCREEN_ICON_TEXTURE_H_
