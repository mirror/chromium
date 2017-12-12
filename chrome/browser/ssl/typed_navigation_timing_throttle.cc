// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/typed_navigation_timing_throttle.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "ui/base/page_transition_types.h"

TypedNavigationTimingThrottle::TypedNavigationTimingThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}

// static
std::unique_ptr<content::NavigationThrottle>
TypedNavigationTimingThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!handle->GetURL().SchemeIs("http"))
    return nullptr;

  return base::WrapUnique(new TypedNavigationTimingThrottle(handle));
}

TypedNavigationTimingThrottle::~TypedNavigationTimingThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
TypedNavigationTimingThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (ui::PageTransitionCoreTypeIs(navigation_handle()->GetPageTransition(),
                                   ui::PAGE_TRANSITION_TYPED)) {
    started_ = true;
    initial_url_ = navigation_handle()->GetURL();
  }

  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
TypedNavigationTimingThrottle::WillRedirectRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!started_)
    return content::NavigationThrottle::PROCEED;

  // Check if the URL is the same as the original but upgraded to HTTPS.
  GURL redirect_url = navigation_handle()->GetURL();
  if (redirect_url.SchemeIs("https") &&
      redirect_url.GetContent() == initial_url_.GetContent()) {
    UMA_HISTOGRAM_TIMES(
        "Omnibox.URLNavigationTimeToRedirectToHTTPS",
        base::TimeTicks::Now() - navigation_handle()->NavigationStart());
  }

  return content::NavigationThrottle::PROCEED;
}

const char* TypedNavigationTimingThrottle::GetNameForLogging() {
  return "TypedNavigationTimingThrottle";
}
