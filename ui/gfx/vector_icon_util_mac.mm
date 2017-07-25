// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/gfx/vector_icon_util_mac.h"

#include "ui/gfx/canvas_skia_paint.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"

namespace gfx {
NSImage* NSImageWithIcon(const VectorIcon& icon,
                         SkColor color,
                         NSSize size,
                         bool flip_x) {
  DCHECK(!NSEqualSizes(size, NSZeroSize));
  DCHECK(!icon.is_empty());
  return [NSImage imageWithSize:size
                        flipped:NO
                 drawingHandler:^(NSRect dstRect) {
                   gfx::CanvasSkiaPaint canvas(dstRect, false);
                   ScopedRTLFlipCanvas scoped_rtl_flip_canvas(
                       &canvas, static_cast<int>(size.width), flip_x);

                   gfx::PaintVectorIcon(&canvas, icon, color);
                   return YES;
                 }];
}

NSImage* NSImageWithIcon(const VectorIcon& icon, SkColor color) {
  CGFloat dimension =
      static_cast<CGFloat>(gfx::GetDefaultSizeOfVectorIcon(icon));
  return NSImageWithIcon(icon, color, NSMakeSize(dimension, dimension), false);
}

}  // namespace gfx
