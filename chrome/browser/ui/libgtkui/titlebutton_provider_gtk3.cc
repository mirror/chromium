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

const char* GetButtonStyleClassFromButtonType(int type) {
  switch (type) {
    case IDR_MINIMIZE:
      return "minimize";
    case IDR_MAXIMIZE:
      return "maximize";
    case IDR_RESTORE:
      return "restore";
    case IDR_CLOSE:
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

const char* GetIconNameFromButtonType(int type) {
  switch (type) {
    case IDR_MINIMIZE:
      return "window-minimize-symbolic";
    case IDR_MAXIMIZE:
      return "window-maximize-symbolic";
    case IDR_RESTORE:
      return "window-restore-symbolic";
    case IDR_CLOSE:
      return "window-close-symbolic";
    default:
      NOTREACHED();
      return "";
  }
}

}  // namespace

namespace libgtkui {

gfx::ImageSkia GetTitlebuttonImage(int type,
                                   views::Button::ButtonState state,
                                   int header_height) {
  auto header_context =
      GetStyleContextFromCss("GtkHeaderBar#headerbar.header-bar.titlebar");
  auto button_context = AppendCssNodeToStyleContext(
      header_context, std::string("GtkButton#button.titlebutton.") +
                          GetButtonStyleClassFromButtonType(type));
  GtkStateFlags button_state = GetGtkStateFlagsFromButtonState(state);
  gtk_style_context_set_state(button_context, button_state);

  const gint icon_size =
      16;  // gtkheaderbar.c uses GTK_ICON_SIZE_MENU, which is 16px.
  GdkScreen* screen = gdk_screen_get_default();
  gint icon_scale = gdk_screen_get_monitor_scale_factor(
      screen, gdk_screen_get_primary_monitor(screen));

  const char* icon_name = GetIconNameFromButtonType(type);
  ScopedGObject<GtkIconInfo> info(gtk_icon_theme_lookup_icon_for_scale(
      gtk_icon_theme_get_for_screen(screen), icon_name, icon_size, icon_scale,
      static_cast<GtkIconLookupFlags>(GTK_ICON_LOOKUP_USE_BUILTIN |
                                      GTK_ICON_LOOKUP_GENERIC_FALLBACK)));
  ScopedGObject<GdkPixbuf> icon_pixbuf(gtk_icon_info_load_symbolic_for_context(
      info, button_context, nullptr, nullptr));

  int icon_width = gdk_pixbuf_get_width(icon_pixbuf) / icon_scale;
  int icon_height = gdk_pixbuf_get_height(icon_pixbuf) / icon_scale;

  // TODO(thomasanderson): Factor in the GtkImage size, border, padding, and
  // margin
  int button_width = icon_width;
  int button_height = icon_height;
  if (GtkVersionCheck(3, 20)) {
    int min_width, min_height;
    gtk_style_context_get(button_context, button_state, "min-width", &min_width,
                          "min-height", &min_height, NULL);
    button_width = std::max(min_width, button_width);
    button_height = std::max(min_height, button_height);
  }
  GtkBorder button_border, button_padding;
  gtk_style_context_get_border(button_context, button_state, &button_border);
  button_width += button_border.left + button_border.right;
  button_height += button_border.top + button_border.bottom;
  gtk_style_context_get_padding(button_context, button_state, &button_padding);
  button_width += button_padding.left + button_padding.right;
  button_height += button_padding.top + button_padding.bottom;

  GtkBorder button_margin;
  gtk_style_context_get_margin(button_context, button_state, &button_margin);
  int button_unconstrained_height =
      button_height + button_margin.top + button_margin.bottom;
  int button_total_width =
      button_width + button_margin.left + button_margin.right;

  // Bound the button's height based on its margin and the header's border and
  // padding
  GtkBorder header_padding;
  gtk_style_context_get_padding(header_context, GTK_STATE_FLAG_NORMAL,
                                &header_padding);
  GtkBorder header_border;
  gtk_style_context_get_border(header_context, GTK_STATE_FLAG_NORMAL,
                               &header_border);

  int available_height =
      header_height - (header_padding.top + header_padding.bottom +
                       header_border.top + header_border.bottom);

  int render_x = 0;
  int render_y = 0;
  if (button_unconstrained_height <= available_height) {
    render_x += button_margin.left;
    render_y += header_border.top + header_padding.top + button_margin.top +
                (available_height - button_unconstrained_height) / 2;
  } else {
    int needed_height = header_border.top + header_padding.top +
                        button_unconstrained_height + header_padding.bottom +
                        header_border.bottom;
    double scale = static_cast<double>(header_height) / needed_height;

    render_x += static_cast<int>(scale * button_margin.left + 0.5f);
    render_y += static_cast<int>(
        scale * (header_border.top + header_padding.top + button_margin.top) +
        0.5f);

    button_width = static_cast<int>(scale * button_width + 0.5f);
    button_height = static_cast<int>(scale * button_height + 0.5f);
    button_total_width = static_cast<int>(scale * button_total_width + 0.5f);
  }

  SkBitmap bitmap;
  bitmap.allocN32Pixels(button_total_width, header_height);
  bitmap.eraseColor(0);

  CairoSurface surface(bitmap);
  cairo_t* cr = surface.cairo();

  if (GtkVersionCheck(3, 11, 3) ||
      (button_state & (GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE))) {
    gtk_render_background(button_context, cr, render_x, render_y, button_width,
                          button_height);
    gtk_render_frame(button_context, cr, render_x, render_y, button_width,
                     button_height);
  }
  cairo_save(cr);
  cairo_scale(cr, 1.0f / icon_scale, 1.0f / icon_scale);
  gtk_render_icon(button_context, cr, icon_pixbuf,
                  icon_scale * (render_x + (button_width - icon_width) / 2),
                  icon_scale * (render_y + (button_height - icon_height) / 2));
  cairo_restore(cr);

  return gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
}

}  // namespace libgtkui
