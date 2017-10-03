// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_WEBVIEW_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_WEBVIEW_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {

class CONTENT_EXPORT WebViewThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);

  ~WebViewThrottle() override;

  // NavigationThrottle implementation:
  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  WebViewThrottle(NavigationHandle* handle);
  static void CheckShouldOverrideUrlLoading(
      base::WeakPtr<WebViewThrottle> throttle);

  base::WeakPtrFactory<WebViewThrottle> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(WebViewThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_WEBVIEW_THROTTLE_H_
