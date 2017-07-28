// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_BAR_TEXTURE_H_
#define CHROME_BROWSER_VR_ELEMENTS_BAR_TEXTURE_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_texture.h"

namespace vr {

class BarTexture : public UiTexture {
 public:
  BarTexture();
  ~BarTexture() override;
  gfx::Size GetPreferredTextureSize(int width) const override;
  gfx::SizeF GetDrawnSize() const override;

  void SetSize(float width, float height);
  void SetColor(SkColor color);

 private:
  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  gfx::SizeF pixel_size_;
  gfx::SizeF size_;
  SkColor color_ = SK_ColorBLACK;

  DISALLOW_COPY_AND_ASSIGN(BarTexture);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_BAR_TEXTURE_H_
