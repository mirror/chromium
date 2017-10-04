// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_WEBVIEW_NAVIGATION_THROTTLE_H_
#define CONTENT_BROWSER_ANDROID_WEBVIEW_NAVIGATION_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "content/common/content_export.h"
#include "content/public/browser/navigation_throttle.h"

#if !defined(OS_ANDROID)
#error "This class must be used only on Android."
#endif

namespace content {

class CONTENT_EXPORT WebViewNavigationThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);

  ~WebViewNavigationThrottle() override;

  // NavigationThrottle implementation:
  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  WebViewNavigationThrottle(NavigationHandle* handle);
  static void CheckShouldOverrideUrlLoading(
      base::WeakPtr<WebViewNavigationThrottle> throttle);

  base::WeakPtrFactory<WebViewNavigationThrottle> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebViewNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_WEBVIEW_NAVIGATION_THROTTLE_H_
