// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_GTK3_BORDER_PAINTER_H_
#define CHROME_BROWSER_UI_LIBGTKUI_GTK3_BORDER_PAINTER_H_

#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/views/border.h"

namespace libgtkui {

class Gtk3BorderPainter : public views::Border {
 public:
  explicit Gtk3BorderPainter(ScopedStyleContext context);
  ~Gtk3BorderPainter() override;

  void Paint(const views::View& view, gfx::Canvas* canvas) override;

  gfx::Insets GetInsets() const override;

  gfx::Size GetMinimumSize() const override;

 private:
  mutable ScopedStyleContext context_;
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_GTK3_BORDER_PAINTER_H_
