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

bool LaunchAppWithUrl(
    const scoped_refptr<const Extension> app,
    const std::string& handler_id,
    content::WebContents* source,
    const navigation_interception::NavigationParams& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  LOG(ERROR) << "0";

  // Redirecting for Bookmark Apps is hidden behind a feature flag.
  if (app->from_bookmark() &&
      !base::FeatureList::IsEnabled(features::kDesktopPWAWindowing)) {
    return false;
  }

  LOG(ERROR) << "1";

  // Redirect top-level navigations only. This excludes iframes and webviews
  // in particular.
  if (source->IsSubframe()) {
    DVLOG(1) << "Cancel redirection: source is a subframe";
    return false;
  }

  LOG(ERROR) << "2";

  // If prerendering, don't launch the app but abort the navigation.
  prerender::PrerenderContents* prerender_contents =
      prerender::PrerenderContents::FromWebContents(source);
  LOG(ERROR) << "3";

  if (prerender_contents) {
    LOG(ERROR) << "4";

    prerender_contents->Destroy(prerender::FINAL_STATUS_NAVIGATION_INTERCEPTED);
    LOG(ERROR) << "5";

    return true;
  }

  LOG(ERROR) << "6";

  // These are guaranteed by CreateThrottleFor below.
  DCHECK(UrlHandlers::CanExtensionHandleUrl(app.get(), params.url()));

  LOG(ERROR) << "7";

  DCHECK(!params.is_post());

  LOG(ERROR) << "8";

  Profile* profile =
      Profile::FromBrowserContext(source->GetBrowserContext());

  LOG(ERROR) << "9";

  if (app->from_bookmark()) {
    LOG(ERROR) << "10";

    // If we are in the same app that is navigating no need to open a new
    // app window.
    Browser* browser = chrome::FindBrowserWithWebContents(source);
    LOG(ERROR) << "11";

    LOG(ERROR) << browser->app_name();
    LOG(ERROR) << "11.2";
    LOG(ERROR) << app->id();
    LOG(ERROR) << "11.3";
    LOG(ERROR) << web_app::GenerateApplicationNameFromExtensionId(app->id());
    LOG(ERROR) << "11.4";

    if (browser->app_name() ==
        web_app::GenerateApplicationNameFromExtensionId(app->id())) {
      LOG(ERROR) << "12";

      return false;
    } else {
      LOG(ERROR) << "13";

      AppLaunchParams launch_params(profile, app.get(),
                                    extensions::LAUNCH_CONTAINER_WINDOW,
                                    WindowOpenDisposition::CURRENT_TAB,
                                    extensions::SOURCE_CHROME_INTERNAL);
      LOG(ERROR) << "14";

      launch_params.override_url = params.url();
      LOG(ERROR) << "15";

      OpenApplication(launch_params);
      LOG(ERROR) << "16";

      return true;
    }
  }

  LOG(ERROR) << "17";

  DVLOG(1) << "Launching app handler with URL: "
           << params.url().spec() << " -> "
           << app->name() << "(" << app->id() << "):" << handler_id;
  LOG(ERROR) << "18";

  apps::LaunchPlatformAppWithUrl(
      profile, app.get(), handler_id, params.url(), params.referrer().url);

  LOG(ERROR) << "19";

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
