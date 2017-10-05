// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_GTK3_BACKGROUND_PAINTER_H_
#define CHROME_BROWSER_UI_LIBGTKUI_GTK3_BACKGROUND_PAINTER_H_

#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/views/background.h"

namespace libgtkui {

class Gtk3BackgroundPainter : public views::Background {
 public:
  explicit Gtk3BackgroundPainter(ScopedStyleContext context);
  ~Gtk3BackgroundPainter() override;

  void Paint(gfx::Canvas* canvas, views::View* view) const override;

 private:
  mutable ScopedStyleContext context_;
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_GTK3_BACKGROUND_PAINTER_H_
