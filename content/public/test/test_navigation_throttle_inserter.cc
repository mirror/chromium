// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_navigation_throttle_inserter.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"

namespace content {

namespace {

class ThrottleInserter : public content::NavigationThrottle {
 public:
  ThrottleInserter(content::NavigationHandle* handle,
                   ThrottleInsertionCallback* callback)
      : content::NavigationThrottle(handle), callback_(callback) {}
  ~ThrottleInserter() override {}

  // NavigationThrottle:
  ThrottleCheckResult WillStartRequest() override {
    DCHECK(callback_);
    if (std::unique_ptr<NavigationThrottle> throttle =
            callback_->Run(navigation_handle())) {
      navigation_handle()->RegisterThrottleForTesting(std::move(throttle));
    }
    return PROCEED;
  }
  const char* GetNameForLogging() override { return "ThrottleInserter"; }

 private:
  ThrottleInsertionCallback* callback_;
};

}  // namespace

TestNavigationThrottleInserter::TestNavigationThrottleInserter(
    WebContents* web_contents,
    ThrottleInsertionCallback callback)
    : WebContentsObserver(web_contents), callback_(std::move(callback)) {}

TestNavigationThrottleInserter::~TestNavigationThrottleInserter() = default;

void TestNavigationThrottleInserter::DidStartNavigation(
    NavigationHandle* navigation_handle) {
  navigation_handle->RegisterThrottleForTesting(
      std::make_unique<ThrottleInserter>(navigation_handle, &callback_));
}

}  // namespace content
