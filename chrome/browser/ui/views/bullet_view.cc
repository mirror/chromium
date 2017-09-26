// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bullet_view.h"
#include "ui/gfx/canvas.h"

BulletView::BulletView(SkColor color) : color_(color) {}

void BulletView::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);

  SkScalar radius = std::min(height(), width()) / 8.0;
  gfx::Point center = GetLocalBounds().CenterPoint();

  SkPath path;
  path.addCircle(center.x(), center.y(), radius);

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kStrokeAndFill_Style);
  flags.setColor(color_);
  flags.setAntiAlias(true);

  canvas->DrawPath(path, flags);
}
