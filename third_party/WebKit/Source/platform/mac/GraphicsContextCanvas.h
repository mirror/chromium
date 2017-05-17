// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GraphicsContextCanvas_h
#define GraphicsContextCanvas_h

#include <ApplicationServices/ApplicationServices.h>

#include "platform/PlatformExport.h"
#include "platform/graphics/paint/PaintCanvas.h"
#include "third_party/skia/include/core/SkBitmap.h"

struct SkIRect;

namespace blink {

// Converts a PaintCanvas temporarily to a CGContext
class PLATFORM_EXPORT GraphicsContextCanvas {
 public:
  // Internally creates a bitmap the same size |paint_rect|, scaled by
  // |bitmap_scale_factor|.  Painting into the CgContext will go into the
  // bitmap.  Upon destruction, that bitmap will be painted into the
  // canvas as the rectangle |paint_rect|.  Users are expected to
  // clip |paint_rect| to reasonable sizes to not create giant bitmaps.
  GraphicsContextCanvas(PaintCanvas*,
                        const SkIRect& paint_rect,
                        SkScalar bitmap_scale_factor = 1);
  ~GraphicsContextCanvas();

  CGContextRef CgContext();

 private:
  void ReleaseIfNeeded();

  PaintCanvas* canvas_;

  CGContextRef cg_context_;
  SkBitmap offscreen_;
  SkScalar bitmap_scale_factor_;

  // If |paint_rect_| is empty, then |bitmap_| is a dummy 1x1 bitmap
  // allocated for the sake of creating a non-null CGContext (it is invalid to
  // use a null CGContext), and will not be copied to |m_canvas|.
  // TODO(enne): It's not clear that this special empty case ever happens.
  SkIRect paint_rect_;
};

}  // namespace blink

#endif  // GraphicsContextCanvas_h
