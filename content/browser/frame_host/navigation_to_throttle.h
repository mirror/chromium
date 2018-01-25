// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_NAVIGATION_TO_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_NAVIGATION_TO_THROTTLE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;

// A NavigationToThrottle checks navigations against the `navigation-to` CSP
// directive.
class CONTENT_EXPORT NavigationToThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);

  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  explicit NavigationToThrottle(NavigationHandle* handle);
  NavigationThrottle::ThrottleCheckResult
  CheckContentSecurityPolicyNavigationTo(bool is_redirect);

  DISALLOW_COPY_AND_ASSIGN(NavigationToThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_NAVIGATION_TO_THROTTLE_H_
