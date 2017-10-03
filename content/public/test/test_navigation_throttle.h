// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_
#define CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {

class NavigationHandle;

// This class can be used to cancel navigations synchronously or asynchronously
// at a specific time in the NavigationThrottle lifecycle.
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

  struct Status {
    Status(ThrottleMethod method,
           ResultSynchrony synchrony,
           NavigationThrottle::ThrottleCheckResult result)
        : method(method), synchrony(synchrony), result(result) {}

    ThrottleMethod method;
    ResultSynchrony synchrony;
    NavigationThrottle::ThrottleCheckResult result;
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

  // Callback to be called when the given method is called.
  void SetCallback(ThrottleMethod method, base::RepeatingClosure callback);

 protected:
  // A method that subclasses can override to be called immediately before a
  // throttle responds, either by returning synchronously, or by calling
  // CancelDeferredNavigation() asynchronously. In the latter case, the method
  // will be called on the UI thread.
  //
  // TODO(crbug.com/770292): Figure out how to use a threadsafe callback
  // instead, and convert ResourceLoadingCancellingThrottle in
  // ads_page_load_metrics_observer_unittest.cc
  void OnWillRespond(NavigationThrottle::ThrottleCheckResult result){};

 private:
  NavigationThrottle::ThrottleCheckResult ProcessMethod(ThrottleMethod method);
  void CancelAsynchronously(NavigationThrottle::ThrottleCheckResult result);

  struct MethodProperties {
   public:
    MethodProperties();
    ~MethodProperties();
    ResultSynchrony synchrony;
    NavigationThrottle::ThrottleCheckResult result;
    base::Optional<base::RepeatingClosure> callback;
  };
  MethodProperties method_properties_[NUM_THROTTLE_METHODS];

  base::WeakPtrFactory<TestNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_CANCELLING_NAVIGATION_THROTTLE_H_
