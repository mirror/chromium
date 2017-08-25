// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/tab_coordinator.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"
#import "ios/chrome/browser/ui/toolbar/test/toolbar_test_web_state.h"
#import "ios/clean/chrome/browser/ui/tab/basic_tab_container_view_controller.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class TabCoordinatorTest : public BrowserCoordinatorTest {
 protected:
  TabCoordinatorTest() {
    // Initialize the web state.
    auto navigation_manager = base::MakeUnique<web::TestNavigationManager>();
    web_state_.SetNavigationManager(std::move(navigation_manager));

    // Initialize the coordinator.
    coordinator_ = [[TabCoordinator alloc] init];
    coordinator_.browser = GetBrowser();
    coordinator_.webState = &web_state_;
  }
  ~TabCoordinatorTest() override {
    // Explicitly disconnect the mediator so there won't be any WebStateList
    // observers when web_state_list_ gets dealloc.
    [coordinator_ disconnect];
    [coordinator_ stop];
    coordinator_ = nil;
  }

 protected:
  TabCoordinator* coordinator_;
  ToolbarTestWebState web_state_;
};

}  // namespace

TEST_F(TabCoordinatorTest, DefaultContainer) {
  [coordinator_ start];
  EXPECT_EQ([BasicTabContainerViewController class],
            [coordinator_.viewController class]);
}
