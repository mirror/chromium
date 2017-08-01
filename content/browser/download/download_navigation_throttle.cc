// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_navigation_throttle.h"

#include "base/memory/ptr_util.h"

namespace content {

// static
std::unique_ptr<NavigationThrottle>
DownloadNavigationThrottle::CreateThrottleForNavigation(
    NavigationHandle* navigation_handle) {
  return base::WrapUnique(new DownloadNavigationThrottle(navigation_handle));
}

DownloadNavigationThrottle::DownloadNavigationThrottle(
    NavigationHandle* navigation_handle)
    : NavigationThrottle(navigation_handle) {}

DownloadNavigationThrottle::~DownloadNavigationThrottle() {}

NavigationThrottle::ThrottleCheckResult
DownloadNavigationThrottle::WillProcessResponse() {
  // TODO(qinmin): intercept the response if this is a download.
  return NavigationThrottle::PROCEED;
}

const char* DownloadNavigationThrottle::GetNameForLogging() {
  return "MixedContentNavigationThrottle";
}

}  // namespace content
