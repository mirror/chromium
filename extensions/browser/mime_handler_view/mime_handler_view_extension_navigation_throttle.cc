// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mime_handler_view/mime_handler_view_extension_navigation_throttle.h"

#include "base/feature_list.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_features.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/mime_handler_view/mime_handler_view_manager_host.h"

namespace extensions {

MimeHandlerViewExtensionNavigationThrottle::
    MimeHandlerViewExtensionNavigationThrottle(
        content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

MimeHandlerViewExtensionNavigationThrottle::
    ~MimeHandlerViewExtensionNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
MimeHandlerViewExtensionNavigationThrottle::WillStartRequest() {
  if (!base::FeatureList::IsEnabled(features::kPdfExtensionInOutOfProcessFrame))
    return content::NavigationThrottle::PROCEED;

  if (!util::IsPdfExtensionUrl(navigation_handle()->GetURL()))
    return content::NavigationThrottle::PROCEED;

  content::RenderFrameHost* parent_frame =
      navigation_handle()->GetParentFrame();
  if (!parent_frame) {
    // A PDF resource is always embedded in the page. So when the <embed>'s
    // content frame is navigating, there has to be a parent node. Therefore,
    // any navigation to PDF extension from omnibox will not be tracked nor
    // stalled.
    return content::NavigationThrottle::PROCEED;
    ;
  }

  if (!MimeHandlerViewManagerHost::Get(parent_frame)->InterceptNavigation(this))
    return content::NavigationThrottle::PROCEED;

  return content::NavigationThrottle::DEFER;
}

const char* MimeHandlerViewExtensionNavigationThrottle::GetNameForLogging() {
  return "MimeHandlerViewExtensionNavigationThrottle";
}

void MimeHandlerViewExtensionNavigationThrottle::DoResume() {
  Resume();
}

void MimeHandlerViewExtensionNavigationThrottle::DoCancelDeferredNavigation(
    content::NavigationThrottle::ThrottleCheckResult result) {
  CancelDeferredNavigation(result);
}

}  // namespace extensions
