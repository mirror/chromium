// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/gtk3_background_painter.h"

#include "chrome/browser/ui/views/button_background_painter_delegate.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace libgtkui {

namespace {

GtkStateFlags ButtonStateToStateFlags(views::Button::ButtonState state) {
  switch (state) {
    case views::Button::STATE_DISABLED:
      return GTK_STATE_FLAG_INSENSITIVE;
    case views::Button::STATE_HOVERED:
      return GTK_STATE_FLAG_PRELIGHT;
    case views::Button::STATE_NORMAL:
      return GTK_STATE_FLAG_NORMAL;
    case views::Button::STATE_PRESSED:
      return static_cast<GtkStateFlags>(GTK_STATE_FLAG_PRELIGHT |
                                        GTK_STATE_FLAG_ACTIVE);
    default:
      NOTREACHED();
      return GTK_STATE_FLAG_NORMAL;
  }
}

}  // namespace

Gtk3BackgroundPainter::Gtk3BackgroundPainter(
    std::unique_ptr<views::ButtonBackgroundPainterDelegate> delegate,
    ScopedStyleContext context)
    : delegate_(std::move(delegate)), context_(std::move(context)) {}

Gtk3BackgroundPainter::~Gtk3BackgroundPainter() {}

void Gtk3BackgroundPainter::Paint(gfx::Canvas* canvas,
                                  views::View* view) const {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(view->width(), view->height());
  bitmap.eraseColor(0);
  CairoSurface surface(bitmap);
  gtk_style_context_save(context_);
  gtk_style_context_set_state(context_, CalculateStateFlags());
  gtk_render_background(context_, surface.cairo(), 0, 0, view->width(),
                        view->height());
  gtk_render_frame(context_, surface.cairo(), 0, 0, view->width(),
                   view->height());
  gtk_style_context_restore(context_);
  canvas->DrawImageInt(gfx::ImageSkia::CreateFrom1xBitmap(bitmap), 0, 0);
}

GtkStateFlags Gtk3BackgroundPainter::CalculateStateFlags() const {
  GtkStateFlags state =
      ButtonStateToStateFlags(delegate_->CalculateButtonState());
  if (!delegate_->GetWidget()->IsActive())
    state = static_cast<GtkStateFlags>(state | GTK_STATE_FLAG_BACKDROP);
  return state;
}

}  // namespace libgtkui
