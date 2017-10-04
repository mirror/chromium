// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_PASSWORD_PROTECTION_PASSWORD_PROTECTION_NAVIGATION_THROTTLE_H_
#define COMPONENTS_SAFE_BROWSING_PASSWORD_PROTECTION_PASSWORD_PROTECTION_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace safe_browsing {
// PasswordProtectionNavigationThrottle defers navigation when
// PasswordProtectionService is waiting for verdict from Safe Browsing backend.
class PasswordProtectionNavigationThrottle
    : public content::NavigationThrottle {
 public:
  PasswordProtectionNavigationThrottle(
      content::NavigationHandle* navigation_handle,
      bool is_warning_showing);
  ~PasswordProtectionNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

  void ResumeNavigation();
  void CancelNavigation(
      content::NavigationThrottle::ThrottleCheckResult result);

 private:
  bool is_warning_showing_;
  DISALLOW_COPY_AND_ASSIGN(PasswordProtectionNavigationThrottle);
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_PASSWORD_PROTECTION_PASSWORD_PROTECTION_NAVIGATION_THROTTLE_H_
