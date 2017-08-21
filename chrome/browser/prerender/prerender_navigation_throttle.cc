// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_navigation_throttle.h"

#include "base/memory/ptr_util.h"
#include "content/public/browser/navigation_handle.h"

namespace prerender {

namespace {

const char* kNameForLogging = "RedirectsWalkNavigationThrottle";

}  // namespace

std::unique_ptr<content::NavigationThrottle> CreateThrottleForRedirectsWalk(
    PrerenderContents* prerender_contents,
    content::NavigationHandle* handle) {
  DCHECK_EQ(Origin::ORIGIN_REDIRECTS_WALK, prerender_contents->origin());
  return base::MakeUnique<RedirectsWalkNavigationThrottle>(handle,
                                                           prerender_contents);
}

RedirectsWalkNavigationThrottle::RedirectsWalkNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    PrerenderContents* prerender_contents)
    : NavigationThrottle(navigation_handle),
      prerender_contents_(prerender_contents) {}
RedirectsWalkNavigationThrottle::~RedirectsWalkNavigationThrottle(){};

content::NavigationThrottle::ThrottleCheckResult
RedirectsWalkNavigationThrottle::WillRedirectRequest() {
  const GURL& url = navigation_handle()->GetURL();
  const GURL& expected_endpoint =
      prerender_contents_->expected_redirect_endpoint();
  if (url == expected_endpoint) {
    prerender_contents_->OnRedirectsWalkDone(true);
    return content::NavigationThrottle::ThrottleCheckResult::CANCEL;
  }
  return content::NavigationThrottle::ThrottleCheckResult::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
RedirectsWalkNavigationThrottle::WillProcessResponse() {
  // We should not have reached this point without going through a redirect
  // to the expected URL, report failure.
  prerender_contents_->OnRedirectsWalkDone(false);
  return content::NavigationThrottle::ThrottleCheckResult::CANCEL;
}

const char* RedirectsWalkNavigationThrottle::GetNameForLogging() {
  return kNameForLogging;
}

}  // namespace prerender
