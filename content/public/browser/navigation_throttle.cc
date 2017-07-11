// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/navigation_throttle.h"

#include "content/browser/frame_host/navigation_handle_impl.h"

namespace content {

NavigationThrottle::NavigationThrottle(NavigationHandle* navigation_handle)
    : navigation_handle_(navigation_handle) {}

NavigationThrottle::~NavigationThrottle() {}

NavigationThrottle::ThrottleCheckResult NavigationThrottle::WillStartRequest() {
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
NavigationThrottle::WillRedirectRequest() {
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
NavigationThrottle::WillProcessResponse() {
  return NavigationThrottle::PROCEED;
}

void NavigationThrottle::Resume() {
  auto* impl = static_cast<NavigationHandleImpl*>(navigation_handle_);
  CHECK_EQ(this, impl->GetDeferringThrottle());
  impl->Resume();
}

void NavigationThrottle::CancelDeferredNavigation(
    NavigationThrottle::ThrottleCheckResult result) {
  auto* impl = static_cast<NavigationHandleImpl*>(navigation_handle_);
  CHECK_EQ(this, impl->GetDeferringThrottle());
  impl->CancelDeferredNavigation(result);
}

}  // namespace content
