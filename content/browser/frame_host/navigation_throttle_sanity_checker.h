// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_NAVIGATION_THROTTLE_SANITY_CHECKER_H_
#define CONTENT_BROWSER_FRAME_HOST_NAVIGATION_THROTTLE_SANITY_CHECKER_H_

#include <memory>

#include "base/macros.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;

// An NavigationThrottleSanityChecker is responsible for checking the
// NavigationThrottle's methods are called in the right order and the
// NavigationHandle remains consistent.
class CONTENT_EXPORT NavigationThrottleSanityChecker
    : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);
  ~NavigationThrottleSanityChecker() override;

  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  NavigationThrottleSanityChecker(NavigationHandle* handle);
  DISALLOW_COPY_AND_ASSIGN(NavigationThrottleSanityChecker);
  enum class State {
    Initial,
    StartRequestCalled,
    ProcessResponseCalled,
  };
  State state_ = State::Initial;
  bool was_redirected_ = false;
  GURL url_;
  base::WeakPtr<NavigationHandleImpl> navigation_handle_weak_ptr;
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_NAVIGATION_THROTTLE_SANITY_CHECKER_H_
