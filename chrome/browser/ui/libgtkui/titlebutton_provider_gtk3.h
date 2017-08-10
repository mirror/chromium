// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_

#include <map>

#include "base/memory/singleton.h"
#include "chrome/browser/ui/libgtkui/libgtkui_export.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/window/frame_buttons.h"

namespace libgtkui {

class LIBGTKUI_EXPORT TitlebuttonProviderGtk3 {
 public:
  static TitlebuttonProviderGtk3* GetInstance();

  ~TitlebuttonProviderGtk3();

  gfx::ImageSkia GetTitlebuttonImage(views::FrameButtonDisplayType type,
                                     views::Button::ButtonState state,
                                     int top_area_height);

  // Get the external margin around each button.  The left inset
  // represents the leading margin, and the right inset represents the
  // trailing margin.
  gfx::Insets GetWindowCaptionMargin(views::FrameButtonDisplayType type) const;

  // Get the internal spacing (padding + border) of the top area.  The
  // left inset represents the leading spacing, and the right inset
  // represents the trailing spacing.
  const gfx::Insets& top_area_spacing() const { return top_area_spacing_; }

 private:
  friend struct base::DefaultSingletonTraits<TitlebuttonProviderGtk3>;

  TitlebuttonProviderGtk3();

  std::map<views::FrameButtonDisplayType, gfx::Insets> button_margins_;

  gfx::Insets top_area_spacing_;
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_PROVIDER_GTK3_H_
