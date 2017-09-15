// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/delayed_subframe_throttle.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"

namespace content {

DelayedSubframeThrottle::DelayedSubframeThrottle(NavigationHandle* handle)
    : NavigationThrottle(handle), is_throttled_(true), weak_factory_(this) {
  LOG(WARNING) << "Throttling url=" << handle->GetURL();
}

// static
std::unique_ptr<DelayedSubframeThrottle>
DelayedSubframeThrottle::MaybeCreateThrottleFor(NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return std::unique_ptr<DelayedSubframeThrottle>(
      new DelayedSubframeThrottle(handle));
}

DelayedSubframeThrottle::~DelayedSubframeThrottle() {}

NavigationThrottle::ThrottleCheckResult
DelayedSubframeThrottle::WillStartRequest() {
  return is_throttled_ ? NavigationThrottle::DEFER
                       : NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
DelayedSubframeThrottle::WillRedirectRequest() {
  return is_throttled_ ? NavigationThrottle::DEFER
                       : NavigationThrottle::PROCEED;
}

const char* DelayedSubframeThrottle::GetNameForLogging() {
  return "DelayedSubframeThrottle";
}

void DelayedSubframeThrottle::Unthrottle() {
  is_throttled_ = false;
  LOG(WARNING) << "Unthrottling url=" << navigation_handle()->GetURL();
}

base::WeakPtr<DelayedSubframeThrottle> DelayedSubframeThrottle::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace content
