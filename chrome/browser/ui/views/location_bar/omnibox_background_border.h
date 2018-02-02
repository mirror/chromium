// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_OMNIBOX_BACKGROUND_BORDER_H_
#define CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_OMNIBOX_BACKGROUND_BORDER_H_

#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"
#include "third_party/skia/include/core/SkColor.h"

namespace gfx {
class Canvas;
class Rect;
}  // namespace gfx

namespace ui {
class NativeTheme;
}  // namespace ui

// OmniboxBackgroundBorder is used specifically for the LocationBarView, and is
// sensitive to whether it should be displayed in a pill or rectangular shape.
class OmniboxBackgroundBorder : public BackgroundWith1PxBorder {
 public:
  OmniboxBackgroundBorder(SkColor background, SkColor border);

  // Whether the OmniboxBackgroundBorder is a pill shape.
  static bool IsRounded();

  // Paints a blue focus ring that draws over the top of the existing border.
  void PaintFocusRing(gfx::Canvas* canvas,
                      ui::NativeTheme* theme,
                      const gfx::Rect& local_bounds);

 protected:
  // BackgroundWith1PxBorder:
  float GetBorderRadiusInternal(int height_in_px) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmniboxBackgroundBorder);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_OMNIBOX_BACKGROUND_BORDER_H_
