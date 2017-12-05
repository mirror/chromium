// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/startup_tasks.h"

#include <memory>

#import "ios/chrome/app/deferred_initialization_runner.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/download/browser_download_service.h"
#include "ios/chrome/browser/download/browser_download_service_factory.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/download/download_controller.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;

// Test fixture for testing StartupTasks class.
class StartupTasksTest : public PlatformTest {
 protected:
  StartupTasksTest() : browser_state_(browser_state_builder_.Build()) {}
  web::TestWebThreadBundle thread_bundle_;
  TestChromeBrowserState::Builder browser_state_builder_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
};

// Tests that scheduleDeferredBrowserStateInitialization: creates
// BrowserDownloadService and sets it as DownloadControllerDelegate.
TEST_F(StartupTasksTest, DownloadService) {
  web::DownloadController* download_controller =
      web::DownloadController::FromBrowserState(browser_state_.get());
  ASSERT_TRUE(download_controller);
  ASSERT_FALSE(download_controller->GetDelegate());

  [DeferredInitializationRunner sharedInstance].delayBeforeFirstBlock = 0;
  [StartupTasks
      scheduleDeferredBrowserStateInitialization:browser_state_.get()];

  ASSERT_TRUE(WaitUntilConditionOrTimeout(testing::kWaitForActionTimeout, ^{
    return download_controller->GetDelegate() != nullptr;
  }));

  BrowserDownloadService* service =
      BrowserDownloadServiceFactory::GetForBrowserState(browser_state_.get());
  EXPECT_EQ(service, download_controller->GetDelegate());
}
