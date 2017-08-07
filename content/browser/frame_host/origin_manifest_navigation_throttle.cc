// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/origin_manifest_navigation_throttle.h"

#include "base/memory/ptr_util.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"

namespace content {

// static
std::unique_ptr<NavigationThrottle>
OriginManifestNavigationThrottle::CreateThrottleForNavigation(
    NavigationHandle* navigation_handle) {
  if (IsBrowserSideNavigationEnabled()) {
    return base::WrapUnique(
        new OriginManifestNavigationThrottle(navigation_handle));
  }
  return nullptr;
}

OriginManifestNavigationThrottle::OriginManifestNavigationThrottle(
    NavigationHandle* navigation_handle)
    : NavigationThrottle(navigation_handle) {
  DCHECK(IsBrowserSideNavigationEnabled());
}

OriginManifestNavigationThrottle::~OriginManifestNavigationThrottle() {}

ThrottleCheckResult OriginManifestNavigationThrottle::WillStartRequest() {
  url_fetcher_ = net::URLFetcher::Create(GURL("http://example.com"),
                                         net::URLFetcher::GET, this);
  // we might need a URLRequestContext here, this is just the first best one for
  // which it does not crash
  url_fetcher_->SetRequestContext(navigation_handle()
                                      ->GetStartingSiteInstance()
                                      ->GetProcess()
                                      ->GetStoragePartition()
                                      ->GetURLRequestContext());
  // we need header(s) here to show we implement the Origin Manifest feature
  // url_fetcher->AddExtraRequestHeader("OriginManifest: 1");
  url_fetcher_->Start();

  return ThrottleCheckResult::DEFER;
}

ThrottleCheckResult OriginManifestNavigationThrottle::WillRedirectRequest() {
  return ThrottleCheckResult::PROCEED;
}

ThrottleCheckResult OriginManifestNavigationThrottle::WillProcessResponse() {
  return ThrottleCheckResult::PROCEED;
}

const char* OriginManifestNavigationThrottle::GetNameForLogging() {
  return "OriginManifestNavigationThrottle";
}

// private
void OriginManifestNavigationThrottle::OnURLFetchComplete(
    const net::URLFetcher* source) {
  std::string response_str;
  if (source->GetResponseAsString(&response_str)) {
    VLOG(1) << response_str;
  } else {
    VLOG(1) << "response not a set as string";
  }
  Resume();
}

}  // namespace content
