// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/security_properties_navigation_throttle.h"

#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/url_constants.h"

namespace content {

SecurityPropertiesNavigationThrottle::SecurityPropertiesNavigationThrottle(
    NavigationHandle* navigation_handle)
    : NavigationThrottle(navigation_handle) {}

SecurityPropertiesNavigationThrottle::~SecurityPropertiesNavigationThrottle() {}

NavigationThrottle::ThrottleCheckResult
SecurityPropertiesNavigationThrottle::WillStartRequest() {
  // Allow only chrome: scheme documents to be navigated to.
  if (navigation_handle()->GetURL().SchemeIs(kChromeUIScheme))
    return PROCEED;

  return BLOCK_REQUEST;
}

const char* SecurityPropertiesNavigationThrottle::GetNameForLogging() {
  return "SecurityPropertiesNavigationThrottle";
}

// static
std::unique_ptr<NavigationThrottle>
SecurityPropertiesNavigationThrottle::CreateThrottleForNavigation(
    NavigationHandle* navigation_handle) {
  // Create the throttle only for subframe navigations.
  if (navigation_handle->IsInMainFrame())
    return nullptr;

  RenderFrameHostImpl* parent =
      static_cast<NavigationHandleImpl*>(navigation_handle)
          ->frame_tree_node()
          ->parent()
          ->current_frame_host();

  // Create a throttle only for navigations in process that has
  // WebUI bindings or the parent frame is at chrome: URL.
  if (ChildProcessSecurityPolicyImpl::GetInstance()->HasWebUIBindings(
          parent->GetProcess()->GetID()) ||
      parent->GetLastCommittedURL().SchemeIs(kChromeUIScheme)) {
    return base::MakeUnique<SecurityPropertiesNavigationThrottle>(
        navigation_handle);
  }

  return nullptr;
}

}  // namespace content
