// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/webview_navigation_throttle.h"

#include "base/bind.h"
#include "build/build_config.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_client.h"

namespace content {

WebViewNavigationThrottle::~WebViewNavigationThrottle() {}

std::unique_ptr<NavigationThrottle>
WebViewNavigationThrottle::MaybeCreateThrottleFor(NavigationHandle* handle) {
  if (IsBrowserSideNavigationEnabled()) {
    return std::unique_ptr<NavigationThrottle>(
        new WebViewNavigationThrottle(handle));
  }

  return nullptr;
}

NavigationThrottle::ThrottleCheckResult
WebViewNavigationThrottle::WillStartRequest() {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CheckShouldOverrideUrlLoading, weak_factory_.GetWeakPtr()));
  return NavigationThrottle::DEFER;
}

NavigationThrottle::ThrottleCheckResult
WebViewNavigationThrottle::WillRedirectRequest() {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CheckShouldOverrideUrlLoading, weak_factory_.GetWeakPtr()));
  return NavigationThrottle::DEFER;
}

const char* WebViewNavigationThrottle::GetNameForLogging() {
  return "WebViewNavigationThrottle";
}

WebViewNavigationThrottle::WebViewNavigationThrottle(NavigationHandle* handle)
    : NavigationThrottle(handle), weak_factory_(this) {}

// static
void WebViewNavigationThrottle::CheckShouldOverrideUrlLoading(
    base::WeakPtr<WebViewNavigationThrottle> throttle) {
  if (!throttle)
    return;

  NavigationHandleImpl* handle =
      static_cast<NavigationHandleImpl*>(throttle->navigation_handle());

  // Redirects are always not counted as from user gesture.
  bool has_user_gesture =
      !handle->WasServerRedirect() && handle->HasUserGesture();

  bool cancel_navigation =
      GetContentClient()->browser()->ShouldOverrideUrlLoading(
          handle->frame_tree_node()->frame_tree_node_id(),
          !handle->IsRendererInitiated(), handle->GetURL(), handle->method(),
          has_user_gesture, handle->WasServerRedirect(),
          handle->IsInMainFrame(), handle->GetPageTransition());

  // ShouldOverrideUrlLoading pauses the browser process and executes the
  // webview's embedder code. It can executes whatever it wants. The navigation
  // might be canceled at this point. See https://crbug.com/770157.
  if (!throttle)
    return;

  if (cancel_navigation)
    throttle->CancelDeferredNavigation(NavigationThrottle::CANCEL);
  else
    throttle->Resume();
}

}  // namespace content
