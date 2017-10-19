// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_SECURITY_PROPERTIES_NAVIGATION_THROTTLE_
#define CONTENT_BROWSER_FRAME_HOST_SECURITY_PROPERTIES_NAVIGATION_THROTTLE_

#include "content/public/browser/navigation_throttle.h"

namespace content {

// This NavigationThrottle class is used to check security properties of
// navigations. The following are enforced:
// * When the parent frame is at chrome:// URL, navigations are allowed only
//   to documents at chrome:// URLs.
// * When the navigation originates from a process that has WebUI bindings,
//   it is only allowed to navigate to chrome:// URLs.
class SecurityPropertiesNavigationThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);

  explicit SecurityPropertiesNavigationThrottle(
      NavigationHandle* navigation_handle);
  ~SecurityPropertiesNavigationThrottle() override;

  // NavigationThrottle methods
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SecurityPropertiesNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_SECURITY_PROPERTIES_NAVIGATION_THROTTLE_
