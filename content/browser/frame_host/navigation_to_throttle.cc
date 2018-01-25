// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/navigation_to_throttle.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/common/browser_side_navigation_policy.h"

namespace content {

NavigationToThrottle::NavigationToThrottle(NavigationHandle* handle)
    : NavigationThrottle(handle) {}

// static
std::unique_ptr<NavigationThrottle>
NavigationToThrottle::MaybeCreateThrottleFor(NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!IsBrowserSideNavigationEnabled())
    return nullptr;

  return std::unique_ptr<NavigationThrottle>(new NavigationToThrottle(handle));
}

NavigationThrottle::ThrottleCheckResult
NavigationToThrottle::WillStartRequest() {
  return CheckContentSecurityPolicyNavigationTo(false /* is_redirect */);
}

NavigationThrottle::ThrottleCheckResult
NavigationToThrottle::WillRedirectRequest() {
  return CheckContentSecurityPolicyNavigationTo(true /* is_redirect */);
}

const char* NavigationToThrottle::GetNameForLogging() {
  return "NavigationToThrottle";
}

NavigationThrottle::ThrottleCheckResult
NavigationToThrottle::CheckContentSecurityPolicyNavigationTo(bool is_redirect) {
  // TODO(arthursonzogni): navigation-to is enforced on the wrong
  // RenderFrameHost. The navigating one is used instead of the one that has
  // initiated the form submission. The renderer side checks are still in place
  // and are used for the moment instead. For redirects, the behavior was
  // already broken before using the browser side checks. See
  // https://crbug.com/700964 and https://crbug.com/798698.

  // In absence of redirects, the target URL is sufficiently verified against
  // the navigation-to CSP by the frame that hosts the form element + initiates
  // form submission + declares the navigation-to CSP (i.e. the same frame does
  // all those 4 things). Because in this scenario there are no frame or
  // renderer boundaries crossed, we don't have to worry about one (potentially
  // compromised) renderer being responsible for enforcing the CSP of another
  // (victim) renderer. Therefore it is okay to return early and do no further
  // browser-side checks.

  if (!is_redirect)
    return NavigationThrottle::PROCEED;

  NavigationHandleImpl* handle =
      static_cast<NavigationHandleImpl*>(navigation_handle());

  // we don't block browser-initiated checks
  if (handle->should_check_main_world_csp() == CSPDisposition::DO_NOT_CHECK ||
      !handle->IsRendererInitiated())
    return NavigationThrottle::PROCEED;

  const GURL& url = handle->GetURL();

  // TODO(arthursonzogni): This is not the right RenderFrameHostImpl. The one
  // that has initiated the navigation must be used instead.
  // See https://crbug.com/700964
  RenderFrameHostImpl* render_frame =
      handle->frame_tree_node()->current_frame_host();

  // TODO(estark): Move this check into NavigationRequest and split it into (1)
  // check report-only CSP, (2) upgrade request if needed, (3) check enforced
  // CSP to match how frame-src works. https://crbug.com/713388
  if (render_frame->IsAllowedByCsp(CSPDirective::NavigationTo, url, is_redirect,
                                   handle->source_location(),
                                   CSPContext::CHECK_ALL_CSP)) {
    return NavigationThrottle::PROCEED;
  }

  return NavigationThrottle::CANCEL;
}

}  // namespace content
