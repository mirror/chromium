// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/security_properties_navigation_throttle.h"

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/console_message_level.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_constants.h"
#include "url/url_constants.h"

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
  // Create the throttle only for subframe navigations in process that has
  // WebUI bindings or the parent frame is at chrome: URL.
  if (navigation_handle->IsInMainFrame())
    return nullptr;

  FrameTreeNode* parent = static_cast<NavigationHandleImpl*>(navigation_handle)
                              ->frame_tree_node()
                              ->parent();

  // TODO(nasko): Not sure whether it should be the FTN navigated or the
  // parent that is used for WebUI bindings check. It shouldn't matter in
  // general, since bindings a processwide and we don't have OOPIFs in WebUI
  // documents.
  if (ChildProcessSecurityPolicyImpl::GetInstance()->HasWebUIBindings(
          parent->current_frame_host()->GetProcess()->GetID()) ||
      parent->current_frame_host()->GetLastCommittedURL().SchemeIs(
          kChromeUIScheme)) {
    return base::MakeUnique<SecurityPropertiesNavigationThrottle>(
        navigation_handle);
  }

  return nullptr;
}

}  // namespace content
