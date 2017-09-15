// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/vector_icon.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"

namespace vr {

void DrawVectorIcon(gfx::Canvas* canvas,
                    const gfx::VectorIcon& icon,
                    float size_px,
                    const gfx::PointF& corner,
                    SkColor color) {
  canvas->Save();
  canvas->Translate({corner.x(), corner.y()});

  // The vector icon system selects 1X icons (icons optimized for 16x16 pixels)
  // or non-1X icons based on device scale factor. VR texture rendering does not
  // concern itself with device scale factor, it just draws at native
  // resolution. So, to ensure that we don't use a 1X icon unless, by luck, we
  // want to draw an icon that is exactly 16x16, we scale our icon here by using
  // device scale factor rather than canvas scale.
  float scale = size_px / GetDefaultSizeOfVectorIcon(icon);
  canvas->OverrideDeviceScaleFactor(scale);
  PaintVectorIcon(canvas, icon, color);
  canvas->OverrideDeviceScaleFactor(1.0);

  canvas->Restore();
}

}  // namespace vr
