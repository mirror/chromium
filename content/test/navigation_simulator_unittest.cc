// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/navigation_simulator.h"

#include <string>
#include <tuple>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_side_navigation_test_utils.h"
#include "content/public/test/cancelling_navigation_throttle.h"
#include "content/public/test/test_browser_context.h"
#include "content/test/test_render_frame_host.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {
enum InitiatedBy { INITIATED_BY_BROWSER, INITIATED_BY_RENDERER };
}

class NavigationSimulatorTest
    : public RenderViewHostImplTestHarness,
      public WebContentsObserver,
      public testing::WithParamInterface<
          std::tuple<InitiatedBy,
                     CancellingNavigationThrottle::CancelTime,
                     CancellingNavigationThrottle::ResultSynchrony>> {
 public:
  NavigationSimulatorTest() {}
  ~NavigationSimulatorTest() override {}

  void SetUp() override {
    std::tie(initiated_by_, cancel_time_, sync_) = GetParam();
    // EnableBrowserSideNavigation() needs to be called before
    // RenderViewHostImplTestHarness::SetUp().
    if (initiated_by_ == INITIATED_BY_BROWSER) {
      LOG(ERROR) << "EnableBrowserSideNavigation";
      EnableBrowserSideNavigation();
    }
    RenderViewHostImplTestHarness::SetUp();
    contents()->GetMainFrame()->InitializeRenderFrameIfNeeded();
    Observe(RenderViewHostImplTestHarness::web_contents());
    switch (initiated_by_) {
      case INITIATED_BY_RENDERER:
        simulator_ = NavigationSimulator::CreateRendererInitiated(
            GURL("https://example.test"), main_rfh());
      case INITIATED_BY_BROWSER:
        simulator_ = NavigationSimulator::CreateBrowserInitiated(
            GURL("https://example.test"),
            RenderViewHostImplTestHarness::web_contents());
    }
  }

  void TearDown() override {
    EXPECT_EQ(expect_navigation_to_finish_, did_finish_navigation_);
    RenderViewHostImplTestHarness::TearDown();
  }

  void DidStartNavigation(content::NavigationHandle* handle) override {
    handle->RegisterThrottleForTesting(
        base::MakeUnique<CancellingNavigationThrottle>(handle, cancel_time_,
                                                       sync_));
  }

  void DidFinishNavigation(content::NavigationHandle* handle) override {
    did_finish_navigation_ = true;
  }

  InitiatedBy initiated_by_;
  CancellingNavigationThrottle::CancelTime cancel_time_;
  CancellingNavigationThrottle::ResultSynchrony sync_;
  std::unique_ptr<NavigationSimulator> simulator_;
  bool expect_navigation_to_finish_ = false;
  bool did_finish_navigation_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(NavigationSimulatorTest);
};

// Stress test the navigation simulator by having a navigation throttle cancel
// the navigation at various points in the flow, both synchronously and
// asynchronously.
TEST_P(NavigationSimulatorTest, Cancel) {
  SCOPED_TRACE(::testing::Message() << "InitiatedBy: " << initiated_by_
                                    << " CancelTime: " << cancel_time_
                                    << " ResultSynchrony: " << sync_);

  expect_navigation_to_finish_ = true;

  simulator_->Start();
  if (cancel_time_ == CancellingNavigationThrottle::WILL_START_REQUEST) {
    EXPECT_EQ(NavigationThrottle::CANCEL,
              simulator_->GetLastThrottleCheckResult());
    return;
  }
  EXPECT_EQ(NavigationThrottle::PROCEED,
            simulator_->GetLastThrottleCheckResult());
  simulator_->Redirect(GURL("https://example.redirect"));
  if (cancel_time_ == CancellingNavigationThrottle::WILL_REDIRECT_REQUEST) {
    EXPECT_EQ(NavigationThrottle::CANCEL,
              simulator_->GetLastThrottleCheckResult());
    return;
  }
  EXPECT_EQ(NavigationThrottle::PROCEED,
            simulator_->GetLastThrottleCheckResult());
  simulator_->Commit();
  if (cancel_time_ == CancellingNavigationThrottle::WILL_PROCESS_RESPONSE) {
    EXPECT_EQ(NavigationThrottle::CANCEL,
              simulator_->GetLastThrottleCheckResult());
    return;
  }
  EXPECT_EQ(NavigationThrottle::PROCEED,
            simulator_->GetLastThrottleCheckResult());
}

INSTANTIATE_TEST_CASE_P(
    CancelMethod,
    NavigationSimulatorTest,
    ::testing::Combine(
        ::testing::Values(INITIATED_BY_BROWSER, INITIATED_BY_RENDERER),
        ::testing::Values(CancellingNavigationThrottle::WILL_START_REQUEST,
                          CancellingNavigationThrottle::WILL_REDIRECT_REQUEST,
                          CancellingNavigationThrottle::WILL_PROCESS_RESPONSE,
                          CancellingNavigationThrottle::NEVER),
        ::testing::Values(CancellingNavigationThrottle::SYNCHRONOUS,
                          CancellingNavigationThrottle::ASYNCHRONOUS)));

// Create a class alias so that gtest doesn't run all tests with all parameters.
typedef NavigationSimulatorTest NavigationSimulatorTestForFail;

// Test canceling the simulated navigation.
TEST_P(NavigationSimulatorTestForFail, CancelFail) {
  simulator_->Start();
  simulator_->Fail(net::ERR_CERT_DATE_INVALID);
  EXPECT_EQ(NavigationThrottle::CANCEL,
            simulator_->GetLastThrottleCheckResult());
}

INSTANTIATE_TEST_CASE_P(
    CancelFail,
    NavigationSimulatorTestForFail,
    ::testing::Combine(
        ::testing::Values(INITIATED_BY_BROWSER),
        ::testing::Values(CancellingNavigationThrottle::WILL_FAIL_REQUEST),
        ::testing::Values(CancellingNavigationThrottle::SYNCHRONOUS,
                          CancellingNavigationThrottle::ASYNCHRONOUS)));

}  // namespace content
