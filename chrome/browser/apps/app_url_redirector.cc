// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_url_redirector.h"

#include "apps/launcher.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "chrome/browser/prerender/prerender_contents.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "components/navigation_interception/intercept_navigation_throttle.h"
#include "components/navigation_interception/navigation_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
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

std::string DispositionToString(WindowOpenDisposition disposition) {
  switch (disposition) {
    case WindowOpenDisposition::UNKNOWN:
      return "Unknown";
    case WindowOpenDisposition::CURRENT_TAB:
      return "current tab";
    case WindowOpenDisposition::SINGLETON_TAB:
      return "singleton tab";
    case WindowOpenDisposition::NEW_FOREGROUND_TAB:
      return "new foreground tab";
    case WindowOpenDisposition::NEW_BACKGROUND_TAB:
      return "new background tab";
    case WindowOpenDisposition::NEW_POPUP:
      return "new popup";
    case WindowOpenDisposition::NEW_WINDOW:
      return "new window";
    case WindowOpenDisposition::SAVE_TO_DISK:
      return "save to disk";
    case WindowOpenDisposition::OFF_THE_RECORD:
      return "off the record";
    case WindowOpenDisposition::IGNORE_ACTION:
      return "ignore action";
  }
  NOTREACHED();
  return "unknown";
}

bool ShouldOverrideNavigation(
    const Extension* app,
    content::WebContents* source,
    const navigation_interception::NavigationParams& params) {
  LOG(ERROR) << "ShouldOverrideNavigation called for: " << params.url();

  ui::PageTransition transition_type = params.transition_type();
  if (!(PageTransitionCoreTypeIs(transition_type, ui::PAGE_TRANSITION_LINK))) {
    LOG(ERROR) << "Don't override: Transition type is "
               << PageTransitionGetCoreTransitionString(transition_type);
    return false;
  }

  // CURRENT_TAB is used when clicking on links that just navigate the frame or
  // clicking on links that open a new tab/window e.g. target=_blank links
  // (clicking on a target=_blank link causes the renderer to create a new
  // window which is then navigated with a CURRENT_TAB disposition).
  // These are navigations we want to intercept so check for them.
  //
  // NEW_WINDOW is used when pressing shift and clicking a link or when clicking
  // "Open in new window" in the context menu. We want to intercept these
  // navigations but only if they come from an app.
  // TODO(crbug.com/758774): Don't intercept NEW_WINDOW navigations in regular
  // websites.
  if (params.disposition() != WindowOpenDisposition::CURRENT_TAB &&
      params.disposition() != WindowOpenDisposition::NEW_WINDOW) {
    LOG(ERROR) << "Don't override: Disposition is "
               << DispositionToString(params.disposition());
    return false;
  }

  Browser* browser = chrome::FindBrowserWithWebContents(source);
  if (browser == nullptr) {
    DVLOG(1) << "Don't override: No browser, can't know if already in app.";
    return false;
  }

  if (browser->app_name() ==
      web_app::GenerateApplicationNameFromExtensionId(app->id())) {
    DVLOG(1) << "Don't override: Already in app.";
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
