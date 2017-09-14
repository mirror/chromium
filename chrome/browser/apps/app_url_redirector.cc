// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_url_redirector.h"

#include <tuple>

#include "apps/launcher.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "chrome/browser/prerender/prerender_contents.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/extra_navigation_info_utils.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "components/navigation_interception/intercept_navigation_throttle.h"
#include "components/navigation_interception/navigation_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;
using content::WebContents;
using extensions::Extension;
using extensions::UrlHandlers;
using extensions::UrlHandlerInfo;

namespace {

bool ShouldOverrideNavigation(
    const Extension* app,
    content::WebContents* source,
    const navigation_interception::NavigationParams& params) {

  ui::PageTransition transition_type = params.transition_type();

  extensions::ExtensionNavigationUIData* data =
      static_cast<ChromeNavigationUIData*>(params.navigation_ui_data())
          ->GetExtensionNavigationUIData();

  WindowOpenDisposition disposition = data->disposition();
  bool had_target_contents = data->had_target_contents();
  bool is_from_app = data->is_from_app();

  DVLOG(1) << "ShouldOverrideNavigation called for: " << params.url();
  DVLOG(1) << "Transition: "
           << PageTransitionGetCoreTransitionString(transition_type);
  DVLOG(1) << "Disposition: " << ui::DispositionToString(disposition);
  DVLOG(1) << "Had target contents: " << had_target_contents;

  if (!(PageTransitionCoreTypeIs(transition_type, ui::PAGE_TRANSITION_LINK))) {
    DVLOG(1) << "Don't override: Transition type is "
             << PageTransitionGetCoreTransitionString(transition_type);
    return false;
  }

  // CURRENT_TAB is used when clicking on links that just navigate the frame.
  // We always want to intercept these navigations.
  //
  // FOREGROUND_TAB is used when clicking on links that open a new tab in the
  // foreground e.g. target=_blank links, trying to open a tab inside an app
  // window when there are no regular browser windows, Ctrl + Shift + Clicking
  // a link, etc. We sometimes want to intercept these navigations; see if
  // statement below.
  //
  // NEW_WINDOW is used when shift + clicking a link or when clicking
  // "Open in new window" in the context menu. We want to intercept these
  // navigations but only if they come from an app.
  if (disposition != WindowOpenDisposition::CURRENT_TAB &&
      disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB &&
      disposition != WindowOpenDisposition::NEW_WINDOW) {
    DVLOG(1) << "Don't override: Disposition is "
             << ui::DispositionToString(disposition);
    return false;
  }

  // We only want to intercept foreground tab navigations when they had target
  // contents, e.g. when a user clicks a target=_blank link or the page calls
  // window.open(), the renderer creates a new window and passes a target
  // contents. Foreground tab navigations that had no target contents include,
  // Ctrl + Click a link inside an app window when there are no browser windows,
  // and clicking the "Open link in new tab" context menu option.
  if (disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB &&
      !had_target_contents) {
    DVLOG(1) << "Don't override: new foreground tab with no target contents.";
    return false;
  }

  // When a user Shift + Clicks a link inside an app we should open an app
  // instead of a new browser window.
  if (disposition == WindowOpenDisposition::NEW_WINDOW && !is_from_app) {
    DVLOG(1) << "Don't override: new window not from app.";
    return false;
  }

  // Don't redirect same origin navigations. This matches what is done on
  // Android.
  if (source->GetLastCommittedURL().GetOrigin() == params.url().GetOrigin()) {
    DVLOG(1) << "Don't override: Same origin navigation.";
    return false;
  }

  return true;
}

bool LaunchAppWithUrl(
    const scoped_refptr<const Extension> app,
    const std::string& handler_id,
    content::WebContents* source,
    const navigation_interception::NavigationParams& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Redirecting for Bookmark Apps is hidden behind a feature flag.
  if (app->from_bookmark() &&
      !base::FeatureList::IsEnabled(features::kDesktopPWAWindowing)) {
    return false;
  }

  // Redirect top-level navigations only. This excludes iframes and webviews
  // in particular.
  if (source->IsSubframe()) {
    DVLOG(1) << "Cancel redirection: source is a subframe";
    return false;
  }

  // If prerendering, don't launch the app but abort the navigation.
  prerender::PrerenderContents* prerender_contents =
      prerender::PrerenderContents::FromWebContents(source);
  if (prerender_contents) {
    prerender_contents->Destroy(prerender::FINAL_STATUS_NAVIGATION_INTERCEPTED);
    return true;
  }

  // These are guaranteed by CreateThrottleFor below.
  DCHECK(UrlHandlers::CanExtensionHandleUrl(app.get(), params.url()));
  DCHECK(!params.is_post());

  Profile* profile =
      Profile::FromBrowserContext(source->GetBrowserContext());

  if (app->from_bookmark()) {
    if (!ShouldOverrideNavigation(app.get(), source, params))
      return false;

    AppLaunchParams launch_params(
        profile, app.get(), extensions::LAUNCH_CONTAINER_WINDOW,
        WindowOpenDisposition::CURRENT_TAB, extensions::SOURCE_URL_HANDLER);
    launch_params.override_url = params.url();
    OpenApplication(launch_params);
    return true;
  }

  DVLOG(1) << "Launching app handler with URL: "
           << params.url().spec() << " -> "
           << app->name() << "(" << app->id() << "):" << handler_id;
  apps::LaunchPlatformAppWithUrl(
      profile, app.get(), handler_id, params.url(), params.referrer().url);

  return true;
}

}  // namespace

// static
std::unique_ptr<content::NavigationThrottle>
AppUrlRedirector::MaybeCreateThrottleFor(content::NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(1) << "Considering URL for redirection: " << handle->GetURL().spec();

  content::BrowserContext* browser_context =
      handle->GetWebContents()->GetBrowserContext();
  DCHECK(browser_context);

  // Support only GET for now.
  if (handle->IsPost()) {
    DVLOG(1) << "Skip redirection: method is not GET";
    return nullptr;
  }

  if (!handle->GetURL().SchemeIsHTTPOrHTTPS()) {
    DVLOG(1) << "Skip redirection: scheme is not HTTP or HTTPS";
    return nullptr;
  }

  // Never redirect URLs to apps in incognito. Technically, apps are not
  // supported in incognito, but that may change in future.
  // See crbug.com/240879, which tracks incognito support for v2 apps.
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    DVLOG(1) << "Skip redirection: unsupported in incognito";
    return nullptr;
  }

  const extensions::ExtensionSet& enabled_extensions =
      extensions::ExtensionRegistry::Get(browser_context)->enabled_extensions();
  for (extensions::ExtensionSet::const_iterator iter =
           enabled_extensions.begin();
       iter != enabled_extensions.end(); ++iter) {
    const UrlHandlerInfo* handler =
        UrlHandlers::FindMatchingUrlHandler(iter->get(), handle->GetURL());
    if (handler) {
      DVLOG(1) << "Found matching app handler for redirection: "
               << (*iter)->name() << "(" << (*iter)->id() << "):"
               << handler->id;
      return std::unique_ptr<content::NavigationThrottle>(
          new navigation_interception::InterceptNavigationThrottle(
              handle,
              base::Bind(&LaunchAppWithUrl,
                         scoped_refptr<const Extension>(*iter), handler->id)));
    }
  }

  DVLOG(1) << "Skipping redirection: no matching app handler found";
  return nullptr;
}
