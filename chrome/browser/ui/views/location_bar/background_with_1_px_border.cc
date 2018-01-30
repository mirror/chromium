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
  PaintBackground(canvas, get_color(), border_color_,
                  GetBorderRadiusInternal(view->height()),
                  view->GetContentsBounds());
}

float BackgroundWith1PxBorder::GetBorderRadiusInternal(int height) const {
  return kBorderRadius;
}

void BackgroundWith1PxBorder::PaintBackground(gfx::Canvas* canvas,
                                              SkColor background,
                                              SkColor border,
                                              float inner_border_radius,
                                              const gfx::Rect& bounds) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  const float scale = canvas->UndoDeviceScaleFactor();
  gfx::RectF border_rect_f(bounds);
  border_rect_f.Scale(scale);

  // |inner_border_radius| is the border radius on the inside of the
  // |OmniboxBackgroundBorder|, so inset |border_rect_f| to ensure there's
  // enough room to outset the outer |SkRRect| later.
  border_rect_f.Inset(kLocationBarBorderThicknessDip,
                      kLocationBarBorderThicknessDip);

  SkRRect inner_rect(SkRRect::MakeRectXY(gfx::RectFToSkRect(border_rect_f),
                                         inner_border_radius,
                                         inner_border_radius));
  SkRRect outer_rect(inner_rect);
  outer_rect.outset(kLocationBarBorderThicknessDip,
                    kLocationBarBorderThicknessDip);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  flags.setColor(border);
  canvas->sk_canvas()->drawDRRect(outer_rect, inner_rect, flags);

  flags.setColor(background);
  canvas->sk_canvas()->drawRRect(inner_rect, flags);
}
