// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_navigation_throttle.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/download/download_stats.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "net/http/http_response_headers.h"

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

DownloadNavigationThrottle::~DownloadNavigationThrottle() = default;

NavigationThrottle::ThrottleCheckResult
DownloadNavigationThrottle::WillProcessResponse() {
  if (!static_cast<NavigationHandleImpl*>(navigation_handle())->is_download())
    return NavigationThrottle::PROCEED;

  const net::HttpResponseHeaders* response_headers =
      navigation_handle()->GetResponseHeaders();
  if (response_headers)
    RecordDownloadHttpResponseCode(response_headers->response_code());
  return NavigationThrottle::PROCEED;
}

const char* DownloadNavigationThrottle::GetNameForLogging() {
  return "DownloadNavigationThrottle";
}

}  // namespace content
