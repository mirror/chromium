// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_DELAYED_SUBFRAME_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_DELAYED_SUBFRAME_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;

class CONTENT_EXPORT DelayedSubframeThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<DelayedSubframeThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);

  ~DelayedSubframeThrottle() override;

  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

  void Unthrottle();
  base::WeakPtr<DelayedSubframeThrottle> GetWeakPtr();

 private:
  explicit DelayedSubframeThrottle(NavigationHandle* handle);

  bool is_throttled_;

  base::WeakPtrFactory<DelayedSubframeThrottle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DelayedSubframeThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_DELAYED_SUBFRAME_THROTTLE_H_
