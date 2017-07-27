// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PAGE_NAVIGATION_THROTTLE_H_
#define CONTENT_BROWSER_DEVTOOLS_PAGE_NAVIGATION_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
namespace protocol {
class NetworkHandler;
}  // namespace protocol

// Used to allow the DevTools client to optionally cancel navigations via the
// Page.setControlNavigations and Page.processNavigation commands.
class PageNavigationThrottle : public content::NavigationThrottle {
 public:
  PageNavigationThrottle(
      base::WeakPtr<protocol::NetworkHandler> network_handler,
      content::NavigationHandle* navigation_handle);
  ~PageNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  NavigationThrottle::ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;
  void Resume() override;
  void CancelDeferredNavigation(
      NavigationThrottle::ThrottleCheckResult result) override;

  // Tells the PageNavigationThrottle to not throttle anything!
  void AlwaysProceed();

 private:
  // The PageHandler that this navigation throttle is associated with.
  base::WeakPtr<protocol::NetworkHandler> network_handler_;

  // Whether or not a navigation was deferred. If deferred we expect a
  // subsequent call to AlwaysProceed, Resume or CancelNavigationIfDeferred.
  bool navigation_deferred_;

  DISALLOW_COPY_AND_ASSIGN(PageNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PAGE_NAVIGATION_THROTTLE_H_
