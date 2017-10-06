// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_NAVIGATION_THROTTLE_
#define ANDROID_WEBVIEW_BROWSER_AW_NAVIGATION_THROTTLE_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace android_webview {
class AwContentBrowserClient;

class AwNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> CreateThrottleFor(
      content::NavigationHandle* handle,
      AwContentBrowserClient* browser_client);

  ~AwNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

 private:
  AwNavigationThrottle(content::NavigationHandle* handle,
                       AwContentBrowserClient* browser_client);

  content::NavigationThrottle::ThrottleCheckResult
  CheckShouldOverrideUrlLoading();

  AwContentBrowserClient* browser_client_;

  base::WeakPtrFactory<AwNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AwNavigationThrottle);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_NAVIGATION_THROTTLE_
