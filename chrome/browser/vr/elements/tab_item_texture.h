// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_TAB_ITEM_TEXTURE_H_
#define CHROME_BROWSER_VR_ELEMENTS_TAB_ITEM_TEXTURE_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_texture.h"

namespace vr {

class TabItemTexture : public UiTexture {
 public:
  TabItemTexture();
  ~TabItemTexture() override;
  gfx::Size GetPreferredTextureSize(int width) const override;
  gfx::SizeF GetDrawnSize() const override;

  void SetSize(float width, float height);
  void SetTitle(const base::string16& title);
  void SetColor(SkColor color);
  void SetImageId(int image_id);

 private:
  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  gfx::SizeF pixel_size_;
  gfx::SizeF size_;
  SkColor color_ = SK_ColorBLACK;
  base::string16 title_;
  int image_id_ = -1;

  DISALLOW_COPY_AND_ASSIGN(TabItemTexture);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_TAB_ITEM_TEXTURE_H_
