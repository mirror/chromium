// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_PASSWORD_PROTECTION_PASSWORD_PROTECTION_NAVIGATION_THROTTLE_H_
#define COMPONENTS_SAFE_BROWSING_PASSWORD_PROTECTION_PASSWORD_PROTECTION_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace safe_browsing {
// PasswordProtectionNavigationThrottle defers or cancel navigation under the
// following condition:
// (1) if navigations start when there is a LoginReputationRequst in flight,
//     this throttle defers navigations. When the verdict comes back, if the
//     verdict results in showing a modal warning dialog, deferred navigations
//     will be canceled; otherwise, deferred navigation will be resumed.
// (2) if navigations start when there is already a modal warning showing, this
//     this throttle will simply cancel these navigations.
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
