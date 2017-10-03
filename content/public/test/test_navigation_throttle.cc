// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_navigation_throttle.h"

#include "base/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"

namespace content {

TestNavigationThrottle::TestNavigationThrottle(NavigationHandle* handle)
    : NavigationThrottle(handle), weak_ptr_factory_(this) {}

TestNavigationThrottle::~TestNavigationThrottle() {}

NavigationThrottle::ThrottleCheckResult
TestNavigationThrottle::WillStartRequest() {
  return ProcessMethod(WILL_START_REQUEST);
}

NavigationThrottle::ThrottleCheckResult
TestNavigationThrottle::WillRedirectRequest() {
  return ProcessMethod(WILL_REDIRECT_REQUEST);
}

NavigationThrottle::ThrottleCheckResult
TestNavigationThrottle::WillProcessResponse() {
  return ProcessMethod(WILL_PROCESS_RESPONSE);
}

const char* TestNavigationThrottle::GetNameForLogging() {
  return "TestNavigationThrottle";
}

void TestNavigationThrottle::SetResponse(
    ThrottleMethod method,
    ResultSynchrony synchrony,
    NavigationThrottle::ThrottleCheckResult result) {
  CHECK_LE(method, NUM_THROTTLE_METHODS) << "Invalid throttle method";
  if (synchrony == ASYNCHRONOUS) {
    DCHECK(result.action() == NavigationThrottle::CANCEL_AND_IGNORE ||
           result.action() == NavigationThrottle::CANCEL ||
           result.action() == NavigationThrottle::BLOCK_REQUEST_AND_COLLAPSE)
        << "Invalid result for asynchronous response. Must have a valid action "
           "for CancelDeferredNavigation().";
  }
  method_properties_[method].synchrony = synchrony;
  method_properties_[method].result = result;
}

void TestNavigationThrottle::SetResponseForAllMethods(
    ResultSynchrony synchrony,
    NavigationThrottle::ThrottleCheckResult result) {
  for (size_t method = 0; method < NUM_THROTTLE_METHODS; method++) {
    SetResponse(static_cast<ThrottleMethod>(method), synchrony, result);
  }
}

void TestNavigationThrottle::SetMethodCalledCallback(StatusCallback callback) {
  method_called_callback_ = callback;
}

void TestNavigationThrottle::SetMethodWillRespondCallback(
    StatusCallback callback) {
  method_will_respond_callback_ = callback;
}

int TestNavigationThrottle::GetCallCount(ThrottleMethod method) {
  CHECK_LE(method, NUM_THROTTLE_METHODS) << "Invalid throttle method";
  return method_properties_[method].num_times_called;
}

NavigationThrottle::ThrottleCheckResult TestNavigationThrottle::ProcessMethod(
    ThrottleMethod method) {
  method_properties_[method].num_times_called++;
  ResultSynchrony synchrony = method_properties_[method].synchrony;
  NavigationThrottle::ThrottleCheckResult result =
      method_properties_[method].result;

  if (method_called_callback_.has_value())
    method_called_callback_.value().Run({method, synchrony, result});

  if (synchrony == ASYNCHRONOUS) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&TestNavigationThrottle::TestNavigationThrottle::
                           CancelAsynchronously,
                       weak_ptr_factory_.GetWeakPtr(), method, result));
    return NavigationThrottle::DEFER;
  }

  if (method_will_respond_callback_.has_value())
    method_will_respond_callback_.value().Run({method, synchrony, result});
  return method_properties_[method].result;
}

void TestNavigationThrottle::CancelAsynchronously(
    ThrottleMethod method,
    NavigationThrottle::ThrottleCheckResult result) {
  if (method_will_respond_callback_.has_value())
    method_will_respond_callback_.value().Run({method, ASYNCHRONOUS, result});
  CancelDeferredNavigation(result);
}

}  // namespace content
