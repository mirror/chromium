// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/gtk3_background_painter.h"

#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

namespace libgtkui {

Gtk3BackgroundPainter::Gtk3BackgroundPainter(ScopedStyleContext context)
    : context_(context) {}

Gtk3BackgroundPainter::~Gtk3BackgroundPainter() {}

void Gtk3BackgroundPainter::Paint(gfx::Canvas* canvas,
                                  views::View* view) const {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(view->width(), view->height());
  bitmap.eraseColor(0);
  CairoSurface surface(bitmap);
  gtk_render_background(context_, surface.cairo(), 0, 0, view->width(),
                        view->height());
  canvas->DrawImageInt(gfx::ImageSkia::CreateFrom1xBitmap(bitmap), 0, 0);
}

}  // namespace libgtkui
