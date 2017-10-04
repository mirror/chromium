// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/password_protection/password_protection_navigation_throttle.h"

#include "content/public/browser/navigation_handle.h"

namespace safe_browsing {
PasswordProtectionNavigationThrottle::PasswordProtectionNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    bool is_warning_showing)
    : content::NavigationThrottle(navigation_handle),
      is_warning_showing_(is_warning_showing) {}

PasswordProtectionNavigationThrottle::~PasswordProtectionNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
PasswordProtectionNavigationThrottle::WillStartRequest() {
  if (is_warning_showing_)
    return content::NavigationThrottle::CANCEL_AND_IGNORE;
  LOG(ERROR)<<"Navigation get deferrered "<<navigation_handle()->GetURL();
  return content::NavigationThrottle::DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
PasswordProtectionNavigationThrottle::WillRedirectRequest() {
  if (is_warning_showing_)
    return content::NavigationThrottle::CANCEL_AND_IGNORE;
  return content::NavigationThrottle::DEFER;
}

const char* PasswordProtectionNavigationThrottle::GetNameForLogging() {
  return "PasswordProtectionNavigationThrottle";
}

void PasswordProtectionNavigationThrottle::ResumeNavigation() {
  LOG(ERROR)<<"Navigation resumed "<<navigation_handle()->GetURL();
  Resume();
}

void PasswordProtectionNavigationThrottle::CancelNavigation(
    content::NavigationThrottle::ThrottleCheckResult result) {
  LOG(ERROR)<<"Navigation canceled "<<navigation_handle()->GetURL();
  CancelDeferredNavigation(result);
}

}  // namespace safe_browsing
