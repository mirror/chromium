// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/hosted_app_frame_header_ash.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

HostedAppFrameHeaderAsh::HostedAppFrameHeaderAsh(
    extensions::HostedAppBrowserController* app_controller,
    views::Widget* frame,
    views::View* header_view,
    ash::FrameCaptionButtonContainerView* caption_button_container,
    ash::FrameCaptionButton* back_button)
    : DefaultFrameHeader(frame,
                         header_view,
                         caption_button_container,
                         back_button),
      app_controller_(app_controller) {
  // Hosted apps apply a theme color if specified by the extension.
  base::Optional<SkColor> theme_color = app_controller->GetThemeColor();
  if (theme_color) {
    SkColor opaque_theme_color =
        SkColorSetA(theme_color.value(), SK_AlphaOPAQUE);
    SetFrameColors(opaque_theme_color, opaque_theme_color);
  }
}

HostedAppFrameHeaderAsh::~HostedAppFrameHeaderAsh() {}

// TODO(calamity): Make this elide behavior more sophisticated in handling other
// possible translations and languages (the domain should always render).
void HostedAppFrameHeaderAsh::PaintTitleBar(gfx::Canvas* canvas) {
  // TODO(calamity): Investigate localization implications of using a separator
  // like this.
  constexpr char kSeparator[] = " | ";
  const base::string16 app_name =
      base::UTF8ToUTF16(app_controller_->GetAppShortName());
  const base::string16 domain =
      base::UTF8ToUTF16(app_controller_->GetDomainAndRegistry());
  base::string16 app_and_domain = l10n_util::GetStringFUTF16(
      IDS_HOSTED_APP_NAME_AND_DOMAIN, app_name, domain);

  // If the title matches the app name, don't render the title.
  base::string16 title = app_controller_->GetTitle();
  if (title == app_name)
    title = base::string16();

  // Add a separator if the title isn't empty.
  if (!title.empty())
    app_and_domain = base::ASCIIToUTF16(kSeparator) + app_and_domain;

  const gfx::Rect available_title_bounds = GetAvailableTitleBounds();
  int title_width = gfx::Canvas::GetStringWidth(title, GetTitleFontList());
  int app_and_domain_width =
      gfx::Canvas::GetStringWidth(app_and_domain, GetTitleFontList());

  // The title is either its own width if it fits, or the space remaining after
  // rendering the app and domain.
  gfx::Rect title_bounds = available_title_bounds;
  title_bounds.set_width(
      std::min(title_bounds.width() - app_and_domain_width, title_width));
  title_bounds.set_x(view()->GetMirroredXForRect(title_bounds));
  canvas->DrawStringRect(title, GetTitleFontList(), GetTitleColor(),
                         title_bounds);

  // The app and domain are placed to the right of the title and clipped to the
  // original title bounds. This string is given full width whenever possible.
  gfx::Rect app_and_domain_bounds = available_title_bounds;
  app_and_domain_bounds.set_x(title_bounds.right());
  app_and_domain_bounds.set_width(app_and_domain_width);
  app_and_domain_bounds.Intersect(available_title_bounds);
  app_and_domain_bounds.set_x(
      view()->GetMirroredXForRect(app_and_domain_bounds));
  canvas->DrawStringRectWithFlags(app_and_domain, GetTitleFontList(),
                                  GetTitleColor(), app_and_domain_bounds,
                                  gfx::Canvas::TEXT_ALIGN_RIGHT);
}
