// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/background_with_1_px_border.h"

#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkPath.h"
#include "third_party/skia/include/pathops/SkPathOps.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

BackgroundWith1PxBorder::BackgroundWith1PxBorder(SkColor background,
                                                 SkColor border)
    : border_color_(border) {
  SetNativeControlColor(background);
}

void BackgroundWith1PxBorder::Paint(gfx::Canvas* canvas,
                                    views::View* view) const {
  gfx::RectF border_rect_f(view->GetContentsBounds());

  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();
  border_rect_f.Scale(scale);

  SkRRect round_rect;
  const SkScalar outer_border_radius =
      SkFloatToScalar(GetBorderRadiusInternal(border_rect_f.height() * scale));
  round_rect.setRectXY(gfx::RectFToSkRect(border_rect_f), outer_border_radius,
                       outer_border_radius);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);

  // Draw a round rect for the stroke, then shrink by the stroke width and
  // paint again for the fill.
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(border_color_);
  canvas->sk_canvas()->drawRRect(round_rect, flags);

  // std::floor is used here in order to snap the fill border to the pixel grid.
  const SkScalar inset_amount =
      std::floor(kLocationBarBorderThicknessDip * scale);
  round_rect.inset(inset_amount, inset_amount);
  flags.setColor(get_color());
  canvas->sk_canvas()->drawRRect(round_rect, flags);
}

int BackgroundWith1PxBorder::GetBorderRadiusInternal(int height) const {
  return kBorderRadius;
}
