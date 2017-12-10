// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/typed_navigation_timing_throttle.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "ui/base/page_transition_types.h"

namespace content {

TypedNavigationTimingThrottle::TypedNavigationTimingThrottle(
    NavigationHandle* handle)
    : NavigationThrottle(handle) {}

// static
std::unique_ptr<NavigationThrottle>
TypedNavigationTimingThrottle::MaybeCreateThrottleFor(
    NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!ui::PageTransitionCoreTypeIs(handle->GetPageTransition(),
                                    ui::PAGE_TRANSITION_TYPED) ||
      !handle->GetURL().SchemeIs("http")) {
    return nullptr;
  }

  return std::unique_ptr<NavigationThrottle>(
      new TypedNavigationTimingThrottle(handle));
}

TypedNavigationTimingThrottle::~TypedNavigationTimingThrottle() {}

NavigationThrottle::ThrottleCheckResult
TypedNavigationTimingThrottle::WillStartRequest() {
  initial_url_ = navigation_handle()->GetURL();
  return NavigationThrottle::PROCEED;
}

NavigationThrottle::ThrottleCheckResult
TypedNavigationTimingThrottle::WillRedirectRequest() {
  // Check if the URL is the same as the original but upgraded to HTTPS.
  GURL redirect_url = navigation_handle()->GetURL();
  if (redirect_url.SchemeIs("https") &&
      redirect_url.GetContent() == initial_url_.GetContent()) {
    UMA_HISTOGRAM_TIMES(
        "Omnibox.TypedNavigation.TimeToRedirectToHTTPS",
        base::TimeTicks::Now() - navigation_handle()->NavigationStart());
  }
  return NavigationThrottle::PROCEED;
}

const char* TypedNavigationTimingThrottle::GetNameForLogging() {
  return "TypedNavigationTimingThrottle";
}

}  // namespace content
