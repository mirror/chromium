// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRERENDER_PRERENDER_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_PRERENDER_PRERENDER_NAVIGATION_THROTTLE_H_

#include <memory>

#include "chrome/browser/prerender/prerender_contents.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace prerender {

class PrerenderContents;

std::unique_ptr<content::NavigationThrottle> CreateThrottleForRedirectsWalk(
    PrerenderContents* prerender_contents,
    content::NavigationHandle* handle);

class RedirectsWalkNavigationThrottle : public content::NavigationThrottle {
 public:
  RedirectsWalkNavigationThrottle(content::NavigationHandle* navigation_handle,
                                  PrerenderContents* prerender_contents);
  ~RedirectsWalkNavigationThrottle() override;

  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  content::NavigationThrottle::ThrottleCheckResult WillProcessResponse()
      override;
  const char* GetNameForLogging() override;

 private:
  PrerenderContents* prerender_contents_;
};

}  // namespace prerender

#endif  // CHROME_BROWSER_PRERENDER_PRERENDER_NAVIGATION_THROTTLE_H_
