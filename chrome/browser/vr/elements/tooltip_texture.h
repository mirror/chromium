// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_TOOLTIP_TEXTURE_H_
#define CHROME_BROWSER_VR_ELEMENTS_TOOLTIP_TEXTURE_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_texture.h"

namespace vr {

class TooltipTexture : public UiTexture {
 public:
  TooltipTexture();
  ~TooltipTexture() override;
  gfx::Size GetPreferredTextureSize(int width) const override;
  gfx::SizeF GetDrawnSize() const override;

  void SetSize(float width, float height);
  void SetText(const base::string16& text);
  void SetBackgroundColor(SkColor color);
  void SetTextColor(SkColor color);

 private:
  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  gfx::SizeF pixel_size_;
  gfx::SizeF size_;
  base::string16 text_;
  SkColor background_color_ = SK_ColorWHITE;
  SkColor text_color_ = SK_ColorBLACK;

  DISALLOW_COPY_AND_ASSIGN(TooltipTexture);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_TOOLTIP_TEXTURE_H_
