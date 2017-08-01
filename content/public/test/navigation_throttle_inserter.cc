// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/navigation_throttle_inserter.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"

namespace content {

// Gathers data from the NavigationHandle assigned to navigations that start
// with the expected URL.

NavigationThrottleInserter::NavigationThrottleInserter(
    WebContents* web_contents,
    ThrottleInsertionCallback callback)
    : WebContentsObserver(web_contents), callback_(std::move(callback)) {}

NavigationThrottleInserter::~NavigationThrottleInserter() = default;

void NavigationThrottleInserter::DidStartNavigation(
    NavigationHandle* navigation_handle) {
  std::unique_ptr<NavigationThrottle> throttle =
      callback_.Run(navigation_handle);
  if (throttle)
    navigation_handle->RegisterThrottleForTesting(std::move(throttle));
}

}  // namespace content
