// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/titlebutton_provider_gtk3.h"

#include <gtk/gtk.h>

#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "ui/base/glib/scoped_gobject.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/resources/grit/views_resources.h"

namespace {

const char* GetButtonStyleClassFromButtonType(
    views::FrameButtonDisplayType type) {
  switch (type) {
    case views::FRAME_BUTTON_DISPLAY_MINIMIZE:
      return "minimize";
    case views::FRAME_BUTTON_DISPLAY_MAXIMIZE:
      return "maximize";
    case views::FRAME_BUTTON_DISPLAY_RESTORE:
      return "restore";
    case views::FRAME_BUTTON_DISPLAY_CLOSE:
      return "close";
    default:
      NOTREACHED();
      return "";
  }
}

GtkStateFlags GetGtkStateFlagsFromButtonState(
    views::Button::ButtonState state) {
  switch (state) {
    case views::Button::STATE_NORMAL:
      return GTK_STATE_FLAG_NORMAL;
    case views::Button::STATE_HOVERED:
      return GTK_STATE_FLAG_PRELIGHT;
    case views::Button::STATE_PRESSED:
      return static_cast<GtkStateFlags>(GTK_STATE_FLAG_PRELIGHT |
                                        GTK_STATE_FLAG_ACTIVE);
    case views::Button::STATE_DISABLED:
      return GTK_STATE_FLAG_INSENSITIVE;
    default:
      NOTREACHED();
      return GTK_STATE_FLAG_NORMAL;
  }
}

const char* GetIconNameFromButtonType(views::FrameButtonDisplayType type) {
  switch (type) {
    case views::FRAME_BUTTON_DISPLAY_MINIMIZE:
      return "window-minimize-symbolic";
    case views::FRAME_BUTTON_DISPLAY_MAXIMIZE:
      return "window-maximize-symbolic";
    case views::FRAME_BUTTON_DISPLAY_RESTORE:
      return "window-restore-symbolic";
    case views::FRAME_BUTTON_DISPLAY_CLOSE:
      return "window-close-symbolic";
    default:
      NOTREACHED();
      return "";
  }
}

gfx::Insets InsetsFromGtkBorder(const GtkBorder& border) {
  return gfx::Insets(border.top, border.left, border.bottom, border.right);
}

gfx::Insets GetPaddingFromStyleContext(GtkStyleContext* context,
                                       GtkStateFlags state) {
  GtkBorder padding;
  gtk_style_context_get_padding(context, state, &padding);
  return InsetsFromGtkBorder(padding);
}

gfx::Insets GetBorderFromStyleContext(GtkStyleContext* context,
                                      GtkStateFlags state) {
  GtkBorder border;
  gtk_style_context_get_border(context, state, &border);
  return InsetsFromGtkBorder(border);
}

gfx::Insets GetMarginFromStyleContext(GtkStyleContext* context,
                                      GtkStateFlags state) {
  GtkBorder margin;
  gtk_style_context_get_margin(context, state, &margin);
  return InsetsFromGtkBorder(margin);
}

}  // namespace

namespace libgtkui {

// static
TitlebuttonProviderGtk3* TitlebuttonProviderGtk3::GetInstance() {
  return base::Singleton<TitlebuttonProviderGtk3>::get();
}

TitlebuttonProviderGtk3::TitlebuttonProviderGtk3() {}

TitlebuttonProviderGtk3::~TitlebuttonProviderGtk3() {}

gfx::ImageSkia TitlebuttonProviderGtk3::GetTitlebuttonImage(
    views::FrameButtonDisplayType type,
    views::Button::ButtonState state,
    int top_area_height) {
  // TODO(thomasanderson): Handle different Chrome scale factors.
  auto header_context =
      GetStyleContextFromCss("GtkHeaderBar#headerbar.header-bar.titlebar");
  auto button_context = AppendCssNodeToStyleContext(
      header_context, "GtkButton#button.titlebutton");
  gtk_style_context_add_class(button_context,
                              GetButtonStyleClassFromButtonType(type));
  GtkStateFlags button_state = GetGtkStateFlagsFromButtonState(state);
  gtk_style_context_set_state(button_context, button_state);

  const gint kIconSize =
      16;  // gtkheaderbar.c uses GTK_ICON_SIZE_MENU, which is 16px.
  GdkScreen* screen = gdk_screen_get_default();
  gint icon_scale = gdk_screen_get_monitor_scale_factor(
      screen, gdk_screen_get_primary_monitor(screen));

  const char* icon_name = GetIconNameFromButtonType(type);

  ScopedGObject<GtkIconInfo> icon_info(gtk_icon_theme_lookup_icon_for_scale(
      gtk_icon_theme_get_for_screen(screen), icon_name, kIconSize, icon_scale,
      static_cast<GtkIconLookupFlags>(GTK_ICON_LOOKUP_USE_BUILTIN |
                                      GTK_ICON_LOOKUP_GENERIC_FALLBACK)));
  ScopedGObject<GdkPixbuf> icon_pixbuf(gtk_icon_info_load_symbolic_for_context(
      icon_info, button_context, nullptr, nullptr));

  // TODO(thomasanderson): Factor in the GtkImage size, border, padding, and
  // margin
  gfx::Size icon_size(gdk_pixbuf_get_width(icon_pixbuf) / icon_scale,
                      gdk_pixbuf_get_height(icon_pixbuf) / icon_scale);
  gfx::Rect button_rect(icon_size);
  if (GtkVersionCheck(3, 20)) {
    int min_width, min_height;
    gtk_style_context_get(button_context, button_state, "min-width", &min_width,
                          "min-height", &min_height, NULL);
    button_rect.set_width(std::max(button_rect.width(), min_width));
    button_rect.set_height(std::max(button_rect.height(), min_height));
  }

  button_rect.Inset(-GetPaddingFromStyleContext(button_context, button_state));
  button_rect.Inset(-GetBorderFromStyleContext(button_context, button_state));

  gfx::Insets button_margin =
      GetMarginFromStyleContext(button_context, button_state);
  int button_unconstrained_height =
      button_rect.height() + button_margin.top() + button_margin.bottom();
  int button_total_width =
      button_rect.width() + button_margin.left() + button_margin.right();

  // Bound the button's height based on its margin and the header's border and
  // padding
  GtkBorder header_padding;
  gtk_style_context_get_padding(header_context, GTK_STATE_FLAG_NORMAL,
                                &header_padding);
  GtkBorder header_border;
  gtk_style_context_get_border(header_context, GTK_STATE_FLAG_NORMAL,
                               &header_border);

  // TODO: scale
  top_area_spacing_ =
      InsetsFromGtkBorder(header_padding) + InsetsFromGtkBorder(header_border);

  int available_height =
      top_area_height - (header_padding.top + header_padding.bottom +
                         header_border.top + header_border.bottom);

  if (button_unconstrained_height <= available_height) {
    button_rect.set_x(button_margin.left());
    button_rect.set_y(header_border.top + header_padding.top +
                      button_margin.top() +
                      (available_height - button_unconstrained_height) / 2);
  } else {
    int needed_height = header_border.top + header_padding.top +
                        button_unconstrained_height + header_padding.bottom +
                        header_border.bottom;
    double scale = static_cast<double>(top_area_height) / needed_height;

    button_rect = gfx::Rect(
        scale * button_margin.left() + 0.5f,
        scale * (header_border.top + header_padding.top + button_margin.top()) +
            0.5f,
        scale * button_rect.width() + 0.5f,
        scale * button_rect.height() + 0.5f);

    button_total_width = static_cast<int>(scale * button_total_width + 0.5f);
  }

  button_margins_[type] = gfx::Insets(
      0, button_rect.x(),
      button_total_width - button_rect.width() - button_rect.x(), 0);

  SkBitmap bitmap;
  bitmap.allocN32Pixels(button_total_width, top_area_height);
  bitmap.eraseColor(0);

  CairoSurface surface(bitmap);
  cairo_t* cr = surface.cairo();

  if (GtkVersionCheck(3, 11, 3) ||
      (button_state & (GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE))) {
    gtk_render_background(button_context, cr, button_rect.x(), button_rect.y(),
                          button_rect.width(), button_rect.height());
    gtk_render_frame(button_context, cr, button_rect.x(), button_rect.y(),
                     button_rect.width(), button_rect.height());
  }
  cairo_save(cr);
  cairo_scale(cr, 1.0f / icon_scale, 1.0f / icon_scale);
  gtk_render_icon(
      button_context, cr, icon_pixbuf,
      icon_scale *
          (button_rect.x() + (button_rect.width() - icon_size.width()) / 2),
      icon_scale *
          (button_rect.y() + (button_rect.height() - icon_size.height()) / 2));
  cairo_restore(cr);

  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
}

gfx::Insets TitlebuttonProviderGtk3::GetWindowCaptionMargin(
    views::FrameButtonDisplayType type) const {
  return button_margins_.find(type)->second;
}

}  // namespace libgtkui
