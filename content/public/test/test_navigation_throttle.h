// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_
#define CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {

class NavigationHandle;

// This class can be used to cancel navigations synchronously or asynchronously
// at a specific time in the NavigationThrottle lifecycle.
//
// Consider using it in conjunction with NavigationSimulator.
// TODO(clamy): Check if this could be used in unit tests exercising
// NavigationThrottle code.
class CancellingNavigationThrottle : public NavigationThrottle {
 public:
  enum ThrottleMethod {
    WILL_START_REQUEST,
    WILL_REDIRECT_REQUEST,
    WILL_PROCESS_RESPONSE,
    NUM_THROTTLE_METHODS
  };

  enum ResultSynchrony {
    SYNCHRONOUS,
    ASYNCHRONOUS,
  };

  CancellingNavigationThrottle(NavigationHandle* handle,
                               CancelTime cancel_time,
                               ResultSynchrony sync);
  ~CancellingNavigationThrottle() override;

  // NavigationThrottle:
  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  NavigationThrottle::ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

  void SetResult(ThrottleMethod method,
                 NavigationThrottle::ThrottleCheckResult result)

      protected :
      // Will be called before the navigation is cancelled.
      virtual void OnWillCancel();

 private:
  NavigationThrottle::ThrottleCheckResult ProcessState(bool should_cancel);
  void MaybeCancel(bool cancel);

 private:
  const CancelTime cancel_time_;
  const ResultSynchrony sync_;
  const base::RepeatingCallback<void(CancelTime)> method_called_callback;
  const base::Callback<void(CancelTime)> method_will_cancel_callback;

  NavigationThrottle::ThrottleCheckResult[NUM_THROTTLE_METHODS] results_;
  NavigationThrottle::ThrottleCheckResult[NUM_THROTTLE_METHODS] call_counts_;

  base::WeakPtrFactory<CancellingNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CancellingNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_
