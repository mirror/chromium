// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_THROTTLE_H_

#include "base/macros.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLFetcher;
}

namespace content {

using ThrottleCheckResult = NavigationThrottle::ThrottleCheckResult;

class OriginManifestNavigationThrottle : public NavigationThrottle,
                                         private net::URLFetcherDelegate {
 public:
  static std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);

  OriginManifestNavigationThrottle(NavigationHandle* navigation_handle);
  ~OriginManifestNavigationThrottle() override;

  // NavigationThrottle overrides.
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  // Overriden from net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // URLFetcher instance.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_THROTTLE_H_
