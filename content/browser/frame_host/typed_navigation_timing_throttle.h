// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_TYPED_NAVIGATION_TIMING_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_TYPED_NAVIGATION_TIMING_THROTTLE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;

// A TypedNavigationTimingThrottle tracks the timings of a navigation caused by
// a typed URL navigation for an HTTP URL which was then redirected to HTTPS.
class CONTENT_EXPORT TypedNavigationTimingThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);

  ~TypedNavigationTimingThrottle() override;

  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  explicit TypedNavigationTimingThrottle(NavigationHandle* handle);

  GURL initial_url_;

  DISALLOW_COPY_AND_ASSIGN(TypedNavigationTimingThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_TYPED_NAVIGATION_TIMING_THROTTLE_H_
