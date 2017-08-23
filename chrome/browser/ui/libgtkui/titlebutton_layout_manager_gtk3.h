// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_LAYOUT_MANAGER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_LAYOUT_MANAGER_GTK3_H_

#include <string>

#include "base/macros.h"
#include "ui/base/glib/glib_signal.h"

typedef struct _GParamSpec GParamSpec;
typedef struct _GtkSettings GtkSettings;

namespace libgtkui {

class GtkUi;

class TitlebuttonLayoutManagerGtk3 {
 public:
  explicit TitlebuttonLayoutManagerGtk3(GtkUi* delegate);
  ~TitlebuttonLayoutManagerGtk3();

 private:
  void SetWindowButtonOrderingFromGtkLayout(const std::string& gtk_layout);

  CHROMEG_CALLBACK_1(TitlebuttonLayoutManagerGtk3,
                     void,
                     OnDecorationButtonLayoutChanged,
                     GtkSettings*,
                     GParamSpec*);

  CHROMEG_CALLBACK_1(TitlebuttonLayoutManagerGtk3,
                     void,
                     OnThemeChanged,
                     GtkSettings*,
                     GParamSpec*);

  GtkUi* delegate_;

  unsigned long signal_id_;

  DISALLOW_COPY_AND_ASSIGN(TitlebuttonLayoutManagerGtk3);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_TITLEBUTTON_LAYOUT_MANAGER_GTK3_H_
