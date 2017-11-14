// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/hosted_app_header_painter_ash.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view.h"

HostedAppHeaderPainterAsh::HostedAppHeaderPainterAsh(
    extensions::HostedAppBrowserController* app_controller)
    : app_controller_(app_controller) {}

HostedAppHeaderPainterAsh::~HostedAppHeaderPainterAsh() {}

// TODO(calamity): Make this elide behavior more sophisticated in handling other
// possible translations and languages (the domain should always render).
void HostedAppHeaderPainterAsh::PaintTitleBar(gfx::Canvas* canvas) {
  constexpr char kSeparator[] = " | ";
  base::string16 title = app_controller_->GetTitle();
  const base::string16 app_name =
      base::UTF8ToUTF16(app_controller_->GetAppShortName());
  const base::string16 domain = base::UTF8ToUTF16(app_controller_->GetDomain());
  base::string16 app_and_domain = l10n_util::GetStringFUTF16(
      IDS_HOSTED_APP_NAME_AND_DOMAIN, app_name, domain);

  // If the title matches the app name, don't render the title.
  if (title == app_name)
    title = base::string16();

  // Add a separator if the title isn't empty.
  if (!title.empty())
    app_and_domain = base::ASCIIToUTF16(kSeparator) + app_and_domain;

  const gfx::Rect original_title_bounds = GetTitleBounds();
  gfx::Rect title_bounds = original_title_bounds;

  int title_width = gfx::Canvas::GetStringWidth(title, GetTitleFontList());
  int app_and_domain_width =
      gfx::Canvas::GetStringWidth(app_and_domain, GetTitleFontList());

  // The title is either its own width if it fits, or the space remaining after
  // rendering the app and domain.
  title_bounds.set_width(
      std::min(title_bounds.width() - app_and_domain_width, title_width));
  title_bounds.set_x(view()->GetMirroredXForRect(title_bounds));
  canvas->DrawStringRectWithFlags(title, GetTitleFontList(), GetTitleColor(),
                                  title_bounds,
                                  gfx::Canvas::NO_SUBPIXEL_RENDERING);

  // The app and domain are placed to the right of the title and clipped to the
  // original title bounds.
  gfx::Rect app_and_domain_bounds = title_bounds;
  app_and_domain_bounds.set_x(title_bounds.right());
  app_and_domain_bounds.set_width(app_and_domain_width);
  app_and_domain_bounds.Intersect(original_title_bounds);
  app_and_domain_bounds.set_x(
      view()->GetMirroredXForRect(app_and_domain_bounds));
  canvas->DrawStringRectWithFlags(
      app_and_domain, GetTitleFontList(), GetTitleColor(),
      app_and_domain_bounds,
      gfx::Canvas::NO_SUBPIXEL_RENDERING | gfx::Canvas::TEXT_ALIGN_RIGHT);
}
