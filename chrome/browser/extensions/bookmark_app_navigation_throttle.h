// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"
#include "base/memory/weak_ptr.h"

namespace content {
class NavigationHandle;
}

namespace extensions {

class BookmarkAppNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle);

  BookmarkAppNavigationThrottle(content::NavigationHandle* navigation_handle);
  ~BookmarkAppNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  // content::ThrottleCheckResult WillStartRequest() override;
  // content::ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  base::WeakPtrFactory<BookmarkAppNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkAppNavigationThrottle);
};

} // namespace extensions

#endif // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_H_
