// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_navigation_throttle.h"

#include "android_webview/browser/aw_content_browser_client.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_client.h"

namespace android_webview {

AwNavigationThrottle::~AwNavigationThrottle() {}

std::unique_ptr<content::NavigationThrottle>
AwNavigationThrottle::CreateThrottleFor(
    content::NavigationHandle* handle,
    AwContentBrowserClient* browser_client) {
  return std::unique_ptr<content::NavigationThrottle>(
      new AwNavigationThrottle(handle, browser_client));
}

content::NavigationThrottle::ThrottleCheckResult
AwNavigationThrottle::WillStartRequest() {
  return CheckShouldOverrideUrlLoading();
}

content::NavigationThrottle::ThrottleCheckResult
AwNavigationThrottle::WillRedirectRequest() {
  return CheckShouldOverrideUrlLoading();
}

const char* AwNavigationThrottle::GetNameForLogging() {
  return "AwNavigationThrottle";
}

AwNavigationThrottle::AwNavigationThrottle(
    content::NavigationHandle* handle,
    AwContentBrowserClient* browser_client)
    : content::NavigationThrottle(handle),
      browser_client_(browser_client),
      weak_ptr_factory_(this) {}

content::NavigationThrottle::ThrottleCheckResult
AwNavigationThrottle::CheckShouldOverrideUrlLoading() {
  // Redirects are always not counted as from user gesture.
  bool has_user_gesture = !navigation_handle()->WasServerRedirect() &&
                          navigation_handle()->HasUserGesture();

  base::WeakPtr<AwNavigationThrottle> this_ptr = weak_ptr_factory_.GetWeakPtr();
  bool cancel = browser_client_->ShouldOverrideUrlLoading(
      navigation_handle()->GetFrameTreeNodeId(),
      !navigation_handle()->IsRendererInitiated(),
      navigation_handle()->GetURL(), navigation_handle()->GetMethod(),
      has_user_gesture, navigation_handle()->WasServerRedirect(),
      navigation_handle()->IsInMainFrame(),
      navigation_handle()->GetPageTransition());

  // **Warning** |this| may be deleted.
  // ShouldOverrideUrlLoading pauses the browser process and executes the
  // webview's embedder code. It can executes whatever it wants.
  // See https://crbug.com/770157.
  if (this_ptr && cancel) {
    navigation_handle()->SetIsIgnoredByWebView();
  }

  return cancel ? content::NavigationThrottle::CANCEL
                : content::NavigationThrottle::PROCEED;
}

}  // namespace android_webview
