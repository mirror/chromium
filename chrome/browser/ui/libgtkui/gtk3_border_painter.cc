// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/gtk3_border_painter.h"

#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

namespace libgtkui {

Gtk3BorderPainter::Gtk3BorderPainter(ScopedStyleContext context)
    : context_(context) {}

Gtk3BorderPainter::~Gtk3BorderPainter() {}

void Gtk3BorderPainter::Paint(const views::View& view, gfx::Canvas* canvas) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(view.width(), view.height());
  bitmap.eraseColor(0);
  CairoSurface surface(bitmap);
  gtk_render_frame(context_, surface.cairo(), 0, 0, view.width(),
                   view.height());
  canvas->DrawImageInt(gfx::ImageSkia::CreateFrom1xBitmap(bitmap), 0, 0);
}

gfx::Insets Gtk3BorderPainter::GetInsets() const {
  // TODO
  return gfx::Insets(5, 5, 5, 5);
}

gfx::Size Gtk3BorderPainter::GetMinimumSize() const {
  // TODO
  return gfx::Size(10, 10);
}

}  // namespace libgtkui
