// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_navigation_throttle.h"

#include <memory>
#include <tuple>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/prerender/prerender_contents.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/extra_navigation_info_utils.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_navigation_ui_data.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace extensions {

// static
std::unique_ptr<content::NavigationThrottle>
BookmarkAppNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  DVLOG(1) << "Considering URL for interception: "
           << navigation_handle->GetURL().spec();
  if (!navigation_handle->IsInMainFrame()) {
    DVLOG(1) << "Don't intercept: Navigation is not in main frame.";
    return nullptr;
  }

  if (navigation_handle->IsPost()) {
    DVLOG(1) << "Don't intercept: Method is POST.";
    return nullptr;
  }

  content::BrowserContext* browser_context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    DVLOG(1) << "Don't intercept: Navigation is in incognito.";
    return nullptr;
  }

  DVLOG(1) << "Attaching Bookmark App Navigation Throttle.";
  return std::make_unique<extensions::BookmarkAppNavigationThrottle>(
      navigation_handle);
}

BookmarkAppNavigationThrottle::BookmarkAppNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

BookmarkAppNavigationThrottle::~BookmarkAppNavigationThrottle() {}

const char* BookmarkAppNavigationThrottle::GetNameForLogging() {
  return "BookmarkAppNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppNavigationThrottle::WillStartRequest() {
  return CheckNavigation();
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppNavigationThrottle::CheckNavigation() {
  content::WebContents* source = navigation_handle()->GetWebContents();
  ui::PageTransition transition_type = navigation_handle()->GetPageTransition();
  if (!(PageTransitionCoreTypeIs(transition_type, ui::PAGE_TRANSITION_LINK))) {
    DVLOG(1) << "Don't intercept: Transition type is "
             << PageTransitionGetCoreTransitionString(transition_type);
    return content::NavigationThrottle::PROCEED;
  }

  if (!navigation_handle()->GetURL().SchemeIsHTTPOrHTTPS()) {
    DVLOG(1) << "Don't intercept: scheme is not HTTP or HTTPS.";
    return content::NavigationThrottle::PROCEED;
  }

  // Don't redirect same origin navigations. This matches what is done on
  // Android.
  DCHECK(navigation_handle()->IsInMainFrame());
  if (source->GetMainFrame()->GetLastCommittedOrigin().IsSameOriginWith(
          url::Origin(navigation_handle()->GetURL()))) {
    DVLOG(1) << "Don't intercept: Same origin navigation.";
    return content::NavigationThrottle::PROCEED;
  }

  content::BrowserContext* browser_context = source->GetBrowserContext();
  scoped_refptr<const extensions::Extension> matching_app;
  for (scoped_refptr<const extensions::Extension> app :
       ExtensionRegistry::Get(browser_context)->enabled_extensions()) {
    if (!app->from_bookmark())
      continue;

    const UrlHandlerInfo* url_handler = UrlHandlers::FindMatchingUrlHandler(
        app.get(), navigation_handle()->GetURL());
    if (!url_handler)
      continue;

    matching_app = app;
    break;
  }

  if (!matching_app) {
    DVLOG(1) << "No matching Bookmark App for URL: "
             << navigation_handle()->GetURL();
    return NavigationThrottle::PROCEED;
  } else {
    DVLOG(1) << "Found matching Bookmark App: " << matching_app->name() << "("
             << matching_app->id() << ")";
  }

  // If prerendering, don't launch the app but abort the navigation.
  prerender::PrerenderContents* prerender_contents =
      prerender::PrerenderContents::FromWebContents(source);
  if (prerender_contents) {
    prerender_contents->Destroy(prerender::FINAL_STATUS_NAVIGATION_INTERCEPTED);
    return content::NavigationThrottle::CANCEL_AND_IGNORE;
  }

  extensions::ExtensionNavigationUIData* data =
      static_cast<ChromeNavigationUIData*>(
          navigation_handle()->GetNavigationUIData())
          ->GetExtensionNavigationUIData();

  WindowOpenDisposition disposition = data->disposition();
  bool had_target_contents = data->had_target_contents();
  bool is_from_app = data->is_from_app();

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
    // TODO(BEFORE LANDING): Uncomment once https://crrev.com/c/701814 lands.
    // DVLOG(1) << "Don't intercept: Disposition is "
    //            << ui::DispositionToString(disposition);
    return content::NavigationThrottle::PROCEED;
  }

  // We only want to intercept foreground tab navigations when they had target
  // contents, e.g. when a user clicks a target=_blank link or the page calls
  // window.open(), the renderer creates a new window and passes a target
  // contents. Foreground tab navigations that had no target contents include,
  // Ctrl + Click a link inside an app window when there are no browser windows,
  // and clicking the "Open link in new tab" context menu option.
  if (disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB &&
      !had_target_contents) {
    DVLOG(1) << "Don't intercept: new foreground tab with no target contents.";
    return content::NavigationThrottle::PROCEED;
  }

  // When a user Shift + Clicks a link inside an app we should open an app
  // instead of a new browser window.
  if (disposition == WindowOpenDisposition::NEW_WINDOW && !is_from_app) {
    DVLOG(1) << "Don't intercept: new window not from app.";
    return content::NavigationThrottle::PROCEED;
  }

  OpenBookmarkApp(matching_app);
  return content::NavigationThrottle::CANCEL_AND_IGNORE;
}

void BookmarkAppNavigationThrottle::OpenBookmarkApp(
    scoped_refptr<const Extension> bookmark_app) {
  content::WebContents* source = navigation_handle()->GetWebContents();
  content::BrowserContext* browser_context = source->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);
  AppLaunchParams launch_params(
      profile, bookmark_app.get(), extensions::LAUNCH_CONTAINER_WINDOW,
      WindowOpenDisposition::CURRENT_TAB, extensions::SOURCE_URL_HANDLER);
  launch_params.override_url = navigation_handle()->GetURL();
  OpenApplication(launch_params);
}

}  // namespace extensions
