// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/omnibox_background_border.h"

#include "base/feature_list.h"
#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkRRect.h"
#include "ui/base/ui_base_features.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/native_theme/native_theme.h"

// static
float OmniboxBackgroundBorder::GetBorderRadius(int height) {
  if (base::FeatureList::IsEnabled(features::kTouchableChrome))
    return height / 2;
  return BackgroundWith1PxBorder::kBorderRadius;
}

OmniboxBackgroundBorder::OmniboxBackgroundBorder(SkColor background,
                                                 SkColor border)
    : BackgroundWith1PxBorder(background, border) {}

// static
void OmniboxBackgroundBorder::PaintFocusRing(gfx::Canvas* canvas,
                                             ui::NativeTheme* theme,
                                             float inner_border_radius,
                                             const gfx::Rect& local_bounds) {
  SkColor focus_ring_color = theme->GetSystemColor(
      ui::NativeTheme::NativeTheme::kColorId_FocusedBorderColor);
  PaintBackground(canvas, SK_ColorTRANSPARENT, focus_ring_color,
                  inner_border_radius, local_bounds);
}

float OmniboxBackgroundBorder::GetBorderRadiusInternal(int height) const {
  return OmniboxBackgroundBorder::GetBorderRadius(height);
}
