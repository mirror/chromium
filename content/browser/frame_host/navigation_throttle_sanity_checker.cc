// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/navigation_throttle_sanity_checker.h"

#include "base/logging.h"

namespace content {

std::unique_ptr<NavigationThrottle>
NavigationThrottleSanityChecker::MaybeCreateThrottleFor(
    NavigationHandle* handle) {
#if defined(NDEBUG)
  return {};
#else
  return std::unique_ptr<NavigationThrottle>(
      new NavigationThrottleSanityChecker(handle));
#endif
}

NavigationThrottleSanityChecker::NavigationThrottleSanityChecker(
    NavigationHandle* handle)
    : NavigationThrottle(handle) {
  navigation_handle_weak_ptr =
      static_cast<NavigationHandleImpl*>(handle)->GetWeakPtrForTesting();
}

NavigationThrottleSanityChecker::~NavigationThrottleSanityChecker() {}

NavigationThrottle::ThrottleCheckResult
NavigationThrottleSanityChecker::WillStartRequest() {
  DCHECK(navigation_handle_weak_ptr);
  DCHECK_EQ(State::Initial, state_);
  state_ = State::StartRequestCalled;
  DCHECK(!navigation_handle()->HasCommitted());
  url_ = navigation_handle()->GetURL();
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
NavigationThrottleSanityChecker::WillRedirectRequest() {
  DCHECK(navigation_handle_weak_ptr);
  DCHECK_EQ(State::StartRequestCalled, state_);
  DCHECK_EQ(true, navigation_handle()->WasServerRedirect());
  was_redirected_ = true;
  url_ = navigation_handle()->GetURL();
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
NavigationThrottleSanityChecker::WillProcessResponse() {
  DCHECK(navigation_handle_weak_ptr);
  DCHECK_EQ(State::StartRequestCalled, state_);
  state_ = State::ProcessResponseCalled;
  DCHECK_EQ(was_redirected_, navigation_handle()->WasServerRedirect());
  DCHECK_EQ(url_, navigation_handle()->GetURL());
  return NavigationThrottle::PROCEED;
}

const char* NavigationThrottleSanityChecker::GetNameForLogging() {
  return "NavigationThrottleSanityChecker";
}

}  // namespace content
