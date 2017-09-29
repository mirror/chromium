// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_ELEMENTS_SYSTEM_INDICATOR_TEXTURE_H_
#define CHROME_BROWSER_VR_ELEMENTS_SYSTEM_INDICATOR_TEXTURE_H_

#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/gfx/vector_icon_types.h"

namespace vr {

class AlertDialogTexture : public UiTexture {
 public:
  AlertDialogTexture(const gfx::VectorIcon& icon,
                     base::string16 text_message,
                     base::string16 second_text_message,
                     base::string16 button_message,
                     base::string16 button2_message);
  explicit AlertDialogTexture(const gfx::VectorIcon& icon);
  ~AlertDialogTexture() override;
  gfx::Size GetPreferredTextureSize(int width) const override;
  gfx::SizeF GetDrawnSize() const override;
  void SetContent(long icon,
                  base::string16 title_text,
                  base::string16 toggle_text,
                  int b_positive,
                  base::string16 b_positive_text,
                  int b_negative,
                  base::string16 b_negative_text);
  void UpdateHoverState(const gfx::PointF& position);

 private:
  void Draw(SkCanvas* canvas, const gfx::Size& texture_size) override;

  gfx::SizeF size_;
  const gfx::VectorIcon& icon_;
  base::string16 title_text_;
  base::string16 toggle_text_;
  base::string16 b_positive_text_;
  base::string16 b_negative_text_;
  bool has_text_;
  SkColor b_positive_color_;
  SkColor b_positive_background_;
  SkColor b_negative_color_;
  SkColor b_negative_background_;

  DISALLOW_COPY_AND_ASSIGN(AlertDialogTexture);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_ELEMENTS_SYSTEM_INDICATOR_TEXTURE_H_
