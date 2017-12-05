// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_SETTINGS_PROVIDER_GTK3_H_
#define CHROME_BROWSER_UI_LIBGTKUI_SETTINGS_PROVIDER_GTK3_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/libgtkui/settings_provider.h"
#include "ui/base/glib/glib_signal.h"

typedef struct _GParamSpec GParamSpec;
typedef struct _GtkSettings GtkSettings;

namespace libgtkui {

class GtkUi;

class SettingsProviderGtk3 : public SettingsProvider {
 public:
  explicit SettingsProviderGtk3(GtkUi* delegate);
  ~SettingsProviderGtk3() override;

 private:
  class SignalContext {};  // TODO

  void SetWindowButtonOrderingFromGtkLayout(const std::string& gtk_layout);

  CHROMEG_CALLBACK_1(SettingsProviderGtk3,
                     void,
                     OnDecorationButtonLayoutChanged,
                     GtkSettings*,
                     GParamSpec*);

  CHROMEG_CALLBACK_1(SettingsProviderGtk3,
                     void,
                     OnMiddleClickActionChanged,
                     GtkSettings*,
                     GParamSpec*);

  CHROMEG_CALLBACK_1(SettingsProviderGtk3,
                     void,
                     OnDoubleClickActionChanged,
                     GtkSettings*,
                     GParamSpec*);

  CHROMEG_CALLBACK_1(SettingsProviderGtk3,
                     void,
                     OnRightClickActionChanged,
                     GtkSettings*,
                     GParamSpec*);

  CHROMEG_CALLBACK_1(SettingsProviderGtk3,
                     void,
                     OnThemeChanged,
                     GtkSettings*,
                     GParamSpec*);

  GtkUi* delegate_;

  unsigned long signal_id_decoration_layout_;

  unsigned long signal_id_middle_click_;

  unsigned long signal_id_double_click_;

  unsigned long signal_id_right_click_;

  DISALLOW_COPY_AND_ASSIGN(SettingsProviderGtk3);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_SETTINGS_PROVIDER_GTK3_H_
