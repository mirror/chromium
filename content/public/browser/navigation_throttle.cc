// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/navigation_throttle.h"

#include "content/browser/frame_host/navigation_handle_impl.h"

namespace content {

NavigationThrottle::ThrottleCheckResult::ThrottleCheckResult(
    NavigationThrottle::ThrottleCheckAction action)
    : action(action), net_error(base::nullopt), error_page_url(base::nullopt) {}

NavigationThrottle::ThrottleCheckResult::ThrottleCheckResult(
    NavigationThrottle::ThrottleCheckAction action,
    int net_error,
    GURL error_page_url)
    : action(action), net_error(net_error), error_page_url(error_page_url) {}

NavigationThrottle::ThrottleCheckResult::ThrottleCheckResult(
    const NavigationThrottle::ThrottleCheckResult& other) = default;

NavigationThrottle::ThrottleCheckResult::~ThrottleCheckResult() {}

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

NavigationThrottle::ThrottleCheckResult NavigationThrottle::WillFailRequest() {
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
NavigationThrottle::WillProcessResponse() {
  return NavigationThrottle::PROCEED;
}

void NavigationThrottle::Resume() {
  static_cast<NavigationHandleImpl*>(navigation_handle_)->Resume(this);
}

void NavigationThrottle::CancelDeferredNavigation(
    NavigationThrottle::ThrottleCheckResult result) {
  static_cast<NavigationHandleImpl*>(navigation_handle_)
      ->CancelDeferredNavigation(this, result);
}

}  // namespace content
