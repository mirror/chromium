// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/hosted_app_browser_controller.h"

#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace extensions {

namespace {

bool IsSameOriginOrMoreSecure(const GURL& app_url, const GURL& page_url) {
  const std::string www("www.");
  return (app_url.scheme_piece() == page_url.scheme_piece() ||
          page_url.scheme_piece() == url::kHttpsScheme) &&
         (app_url.host_piece() == page_url.host_piece() ||
          www + app_url.host() == page_url.host_piece()) &&
         app_url.port() == page_url.port();
}

}  // namespace

// static
bool HostedAppBrowserController::IsForHostedApp(const Browser* browser) {
  const std::string extension_id =
      web_app::GetExtensionIdFromApplicationName(browser->app_name());
  const Extension* extension =
      ExtensionRegistry::Get(browser->profile())->GetExtensionById(
          extension_id, ExtensionRegistry::EVERYTHING);
  return extension && extension->is_hosted_app();
}

// static
bool HostedAppBrowserController::IsForExperimentalHostedAppBrowser(
    const Browser* browser) {
  return base::FeatureList::IsEnabled(features::kDesktopPWAWindowing) &&
         IsForHostedApp(browser);
}

HostedAppBrowserController::HostedAppBrowserController(Browser* browser)
    : browser_(browser),
      extension_id_(
          web_app::GetExtensionIdFromApplicationName(browser->app_name())) {}

HostedAppBrowserController::~HostedAppBrowserController() {}

bool HostedAppBrowserController::ShouldShowLocationBar() const {
  const Extension* extension =
      ExtensionRegistry::Get(browser_->profile())->GetExtensionById(
          extension_id_, ExtensionRegistry::EVERYTHING);

  const content::WebContents* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();

  // Default to not showing the location bar if either |extension| or
  // |web_contents| are null. |extension| is null for the dev tools.
  if (!extension || !web_contents)
    return false;

  if (!extension->is_hosted_app())
    return false;

  // Don't show a location bar until a navigation has occurred.
  if (web_contents->GetLastCommittedURL().is_empty())
    return false;

  const SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  if (helper) {
    security_state::SecurityInfo security_info;
    helper->GetSecurityInfo(&security_info);
    if (security_info.security_level == security_state::DANGEROUS)
      return true;
  }

  GURL launch_url = AppLaunchInfo::GetLaunchWebURL(extension);
  return !(IsSameOriginOrMoreSecure(launch_url,
                                    web_contents->GetVisibleURL()) &&
           IsSameOriginOrMoreSecure(launch_url,
                                    web_contents->GetLastCommittedURL()));
}

void HostedAppBrowserController::UpdateLocationBarVisibility(
    bool animate) const {
  browser_->window()->GetLocationBar()->UpdateLocationBarVisibility(
      ShouldShowLocationBar(), animate);
}

// Returns the extension app icon for an app browser if one exists.
gfx::ImageSkia HostedAppBrowserController::GetExtensionAppIcon() const {
  // TODO(calamity): Use the app name to retrieve the app icon without using the
  // extensions tab helper to make icon load more immediate.
  content::WebContents* contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!contents)
    return gfx::ImageSkia();

  extensions::TabHelper* extensions_tab_helper =
      extensions::TabHelper::FromWebContents(contents);
  if (!extensions_tab_helper)
    return gfx::ImageSkia();

  const SkBitmap* icon_bitmap = extensions_tab_helper->GetExtensionAppIcon();
  if (!icon_bitmap)
    return gfx::ImageSkia();

  return gfx::ImageSkia::CreateFrom1xBitmap(*icon_bitmap);
}

}  // namespace extensions
