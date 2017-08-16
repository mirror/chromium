// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_PROVIDER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_PROVIDER_GTK3_H_

#include "base/memory/singleton.h"
#include "chrome/browser/ui/libgtkui/libgtkui_export.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/custom_button.h"
#include "ui/views/window/frame_buttons.h"

namespace libgtkui {

class LIBGTKUI_EXPORT NavButtonProviderGtk3 {
 public:
  static NavButtonProviderGtk3* GetInstance();

  ~NavButtonProviderGtk3();

  // If |top_area_height| is different from the cached top area
  // height, redraws all button images at the new size.
  void RedrawImages(int top_area_height);

  // Gets the cached button image corresponding to |type| and |state|.
  gfx::ImageSkia GetNavButtonImage(views::FrameButtonDisplayType type,
                                   views::Button::ButtonState state);

  // Gets the external margin around each button.  The left inset
  // represents the leading margin, and the right inset represents the
  // trailing margin.
  gfx::Insets GetWindowCaptionMargin(views::FrameButtonDisplayType type) const;

  // Gets the internal spacing (padding + border) of the top area.
  // The left inset represents the leading spacing, and the right
  // inset represents the trailing spacing.
  const gfx::Insets& top_area_spacing() const { return top_area_spacing_; }

  // Gets the spacing to be used between buttons.
  int inter_button_spacing() const { return inter_button_spacing_; }

 private:
  friend struct base::DefaultSingletonTraits<NavButtonProviderGtk3>;

  NavButtonProviderGtk3();

  int cached_top_area_height_;

  gfx::ImageSkia cached_button_images_[views::FRAME_BUTTON_DISPLAY_COUNT]
                                      [views::CustomButton::STATE_COUNT];

  gfx::Insets button_margins_[views::FRAME_BUTTON_DISPLAY_COUNT];

  gfx::Insets top_area_spacing_;

  int inter_button_spacing_;
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_PROVIDER_GTK3_H_
