// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_EXTENSION_NAVIGATION_THROTTLE_H_
#define EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_EXTENSION_NAVIGATION_THROTTLE_H_

#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/common/child_process_host.h"

namespace content {
class NavigationHandle;
}

namespace extensions {

// This class allows the extensions subsystem to have control over navigations
// and optionally cancel/block them. This is a UI thread class.
class MimeHandlerViewExtensionNavigationThrottle
    : public content::NavigationThrottle {
 public:
  explicit MimeHandlerViewExtensionNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~MimeHandlerViewExtensionNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

  void DoResume();
  void DoCancelDeferredNavigation(
      content::NavigationThrottle::ThrottleCheckResult result);

 private:
  // Shared throttle handler.
  ThrottleCheckResult WillStartOrRedirectRequest();

  int32_t parent_frame_process_id_ =
      content::ChildProcessHost::kInvalidUniqueID;
  int32_t parent_frame_routing_id_ = MSG_ROUTING_NONE;

  DISALLOW_COPY_AND_ASSIGN(MimeHandlerViewExtensionNavigationThrottle);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_MIME_HANDLER_VIEW_MIME_HANDLER_VIEW_EXTENSION_NAVIGATION_THROTTLE_H_
