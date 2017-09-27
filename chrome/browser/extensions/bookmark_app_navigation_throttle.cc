// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_navigation_throttle.h"

namespace extensions {

std::unique_ptr<BookmarkAppNavigationThrottle> MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  VLOG(1) << "Considering URL for Bookmark App Navigation Throttle: "
           << handle->GetURL().spec();
  if (!navigation_handle->IsInMainFrame()) {
    VLOG(1) << "Don't redirect: Navigation is not in main frame.";
    return nullptr;
  }

  if (navigation_handle->IsPost()) {
    VLOG(1) << "Don't redirect: Method is POST.";
    return nullptr;
  }

  content::BrowserContext* browser_context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  DCHECK(browser_context);
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    VLOG(1) << "Don't redirect: Navigation is in incognito.";
    return nullptr;
  }

  VLOG(1) << "Attaching Bookmark App Navigation Throttle.";
  return std::make_unique<extensions::BookmarkAppNavigationThrottle>(navigation_handle);
}

BookmarkAppNavigationThrottle::BookmarkAppNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle),
    weak_ptr_factory_(this) {}

BookmarkAppNavigationThrottle::~BookmarkAppNavigationThrottle() {}

const char* BookmarkAppNavigationThrottle::GetNameForLogging() {
  return "BookmarkAppNavigationThrottle";
}

}  // namespace extensions
