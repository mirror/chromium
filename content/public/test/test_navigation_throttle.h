// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_
#define CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {

class NavigationHandle;

// This class can be used to cancel navigations synchronously or asynchronously
// at a specific time in the NavigationThrottle lifecycle.
//
// Consider using it in conjunction with NavigationSimulator.
// TODO(clamy): Check if this could be used in unit tests exercising
// NavigationThrottle code.
class TestNavigationThrottle : public NavigationThrottle {
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

  TestNavigationThrottle(NavigationHandle* handle);
  ~TestNavigationThrottle() override;

  // NavigationThrottle:
  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  NavigationThrottle::ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

  // Return how often the indicated |method| was called.
  int GetCallCount(ThrottleMethod method);

  // Sets the throttle to respond to the method indicated by |method| using
  // |result|, with the given |synchrony|. This overrides any behaviour
  // previously set for the same |method| using SetResult().
  //
  // If |synchrony| is SYNCHRONOUS:
  // - |result|'s action may be any value except NavigationThrottle::DEFER.
  //
  // If |synchrony| is ASYNCHRONOUS, |result|'s action must be one that that is
  // allowed for NavigationThrottle::CancelDeferredNavigation():
  //  - NavigationThrottle::CANCEL,
  //  - NavigationThrottle::CANCEL_AND_IGNORE, or
  //  - NavigationThrottle::BLOCK_REQUEST_AND_COLLAPSE.
  //
  // At the moment, it is not possible to specify that the throttle should defer
  // and then asynchronously call Resume().
  void SetResponse(ThrottleMethod method,
                   ResultSynchrony synchrony,
                   NavigationThrottle::ThrottleCheckResult result);

 protected:
  // Called when the given method is called on the throttle, before the throttle
  // handles it.
  // virtual void OnMethodCalled(ThrottleMethod method);

  // Will be called right before a throttle responds with the given result,
  // either by returning it synchronously, or by calling
  // CancelDeferredNavigation() asynchronously.
  virtual void OnRespondingWithResult(
      ThrottleMethod method,
      ResultSynchrony synchrony,
      NavigationThrottle::ThrottleCheckResult result);

 private:
  NavigationThrottle::ThrottleCheckResult ProcessMethod(ThrottleMethod method);
  void CancelAsynchronously(ThrottleMethod method,
                            NavigationThrottle::ThrottleCheckResult result);

 private:
  struct MethodProperties {
    ResultSynchrony synchrony = SYNCHRONOUS;
    NavigationThrottle::ThrottleCheckResult result =
        NavigationThrottle::PROCEED;
    int num_times_called = 0;
  };
  MethodProperties method_properties_[NUM_THROTTLE_METHODS];

  base::WeakPtrFactory<TestNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_
