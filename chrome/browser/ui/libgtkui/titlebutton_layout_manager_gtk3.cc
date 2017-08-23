// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/titlebutton_layout_manager_gtk3.h"

#include "base/strings/string_split.h"
#include "chrome/browser/ui/libgtkui/gtk_ui.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"

namespace libgtkui {

namespace {

const char kDefaultGtkLayout[] = "menu:minimize,maximize,close";

std::string GetGtkSettingsStringProperty(GtkSettings* settings,
                                         const gchar* prop_name) {
  GValue layout = G_VALUE_INIT;
  g_value_init(&layout, G_TYPE_STRING);
  g_object_get_property(G_OBJECT(settings), prop_name, &layout);
  DCHECK(G_VALUE_HOLDS_STRING(&layout));
  std::string prop_value(g_value_get_string(&layout));
  g_value_unset(&layout);
  return prop_value;
}

std::string GetDecorationLayoutFromGtkWindow() {
  static ScopedStyleContext context;
  if (!context) {
    context = GetStyleContextFromCss("");
    gtk_style_context_add_class(context, "csd");
  }

  gchar* layout_c = nullptr;
  gtk_style_context_get_style(context, "decoration-button-layout", &layout_c,
                              nullptr);
  DCHECK(layout_c);
  std::string layout(layout_c);
  g_free(layout_c);
  return layout;
}

}  // namespace

TitlebuttonLayoutManagerGtk3::TitlebuttonLayoutManagerGtk3(GtkUi* delegate)
    : delegate_(delegate), signal_id_(0) {
  DCHECK(delegate_);
  GtkSettings* settings = gtk_settings_get_default();
  if (GtkVersionCheck(3, 14)) {
    signal_id_ = g_signal_connect(
        settings, "notify::gtk-decoration-layout",
        G_CALLBACK(OnDecorationButtonLayoutChangedThunk), this);
    DCHECK(signal_id_);
    OnDecorationButtonLayoutChanged(settings, nullptr);
  } else if (GtkVersionCheck(3, 10, 3)) {
    signal_id_ = g_signal_connect_after(settings, "notify::gtk-theme-name",
                                        G_CALLBACK(OnThemeChangedThunk), this);
    DCHECK(signal_id_);
    OnThemeChanged(settings, nullptr);
  } else {
    // On versions older than 3.10.3, the layout was hardcoded.
    SetWindowButtonOrderingFromGtkLayout(kDefaultGtkLayout);
  }
}

TitlebuttonLayoutManagerGtk3::~TitlebuttonLayoutManagerGtk3() {
  if (signal_id_)
    g_signal_handler_disconnect(gtk_settings_get_default(), signal_id_);
}

void TitlebuttonLayoutManagerGtk3::SetWindowButtonOrderingFromGtkLayout(
    const std::string& gtk_layout) {
  std::vector<std::string> sides = base::SplitString(
      gtk_layout, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (sides.size() > 2)
    return;  // TODO: return default value

  while (sides.size() < 2)
    sides.push_back(std::string());

  std::map<std::string, views::FrameButton> button_map{
      {"minimize", views::FRAME_BUTTON_MINIMIZE},
      {"maximize", views::FRAME_BUTTON_MAXIMIZE},
      {"close", views::FRAME_BUTTON_CLOSE}};

  std::vector<views::FrameButton> leading_buttons;
  for (const std::string& button : base::SplitString(
           sides[0], ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (button_map.count(button))
      leading_buttons.push_back(button_map[button]);
  }
  std::vector<views::FrameButton> trailing_buttons;
  for (const std::string& button : base::SplitString(
           sides[1], ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (button_map.count(button))
      trailing_buttons.push_back(button_map[button]);
  }
  delegate_->SetWindowButtonOrdering(leading_buttons, trailing_buttons);
}

void TitlebuttonLayoutManagerGtk3::OnDecorationButtonLayoutChanged(
    GtkSettings* settings,
    GParamSpec* param) {
  SetWindowButtonOrderingFromGtkLayout(
      GetGtkSettingsStringProperty(settings, "gtk-decoration-layout"));
}

void TitlebuttonLayoutManagerGtk3::OnThemeChanged(GtkSettings* settings,
                                                  GParamSpec* param) {
  std::string layout = GetDecorationLayoutFromGtkWindow();

  // Hack to get minimize and maximize buttons on Ambiance, which is
  // the default theme on Ubuntu 14.04.
  if (layout == "close:" &&
      GetGtkSettingsStringProperty(settings, "gtk-theme-name") == "Ambiance") {
    layout = "close,minimize,maximize:";
  }

  SetWindowButtonOrderingFromGtkLayout(layout);
}

}  // namespace libgtkui
