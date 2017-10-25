// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main_content/main_content_coordinator.h"

#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_test.h"
#import "ios/chrome/browser/ui/coordinators/test_browser_coordinator.h"
#import "ios/chrome/browser/ui/main_content/main_content_ui_forwarder.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - TestMainContentViewController

// Test implementation of MainContentViewController.
@interface TestMainContentViewController : UIViewController<MainContentUI>
@end

@implementation TestMainContentViewController
@synthesize UIForwarder = _UIForwarder;

- (instancetype)init {
  if (self = [super init])
    _UIForwarder = [[MainContentUIForwarder alloc] init];
  return self;
}

@end

#pragma mark - TestMainContentCoordinator

// Test version of MainContentCoordinator.
@interface TestMainContentCoordinator : MainContentCoordinator
@end

@implementation TestMainContentCoordinator
@synthesize viewController = _viewController;

- (instancetype)init {
  if (self = [super init])
    _viewController = [[TestMainContentViewController alloc] init];
  return self;
}

@end

#pragma mark - MainContentCoordinatorTest

// Test fixture for MainContentCoordinator.
class MainContentCoordinatorTest : public BrowserCoordinatorTest {
 protected:
  MainContentCoordinatorTest()
      : BrowserCoordinatorTest(),
        parent_([[TestBrowserCoordinator alloc] init]) {
    parent_.browser = GetBrowser();
  }

  // The coordinator to use as the parent for MainContentCoordinators.
  BrowserCoordinator* parent() { return parent_; }

 private:
  BrowserCoordinator* parent_;
  DISALLOW_COPY_AND_ASSIGN(MainContentCoordinatorTest);
};

// Tests that a MainContentCoordinator's ui forwader starts broadcasting
// when started.
TEST_F(MainContentCoordinatorTest, BroadcastAfterStarting) {
  // Start a MainContentCoordinator and verify that its UI is being broadcasted.
  TestMainContentCoordinator* coordinator =
      [[TestMainContentCoordinator alloc] init];
  [parent() addChildCoordinator:coordinator];
  [coordinator start];
  EXPECT_TRUE(coordinator.viewController.UIForwarder.broadcasting);
  // Stop the coordinator and verify that its UI is no longer being broadcasted.
  [coordinator stop];
  [parent() removeChildCoordinator:coordinator];
  EXPECT_FALSE(coordinator.viewController.UIForwarder.broadcasting);
}

// Tests that a MainContentCoordinator's ui forwader stops broadcasting when
// a child MainContentCoordinator is started.
TEST_F(MainContentCoordinatorTest, StopBroadcastingForChildren) {
  // Start a MainContentCoordinator and start a child MainContentCoordinator,
  // then verify that the child's UI is being broadcast, not the parent's.
  TestMainContentCoordinator* parentMainCoordinator =
      [[TestMainContentCoordinator alloc] init];
  [parent() addChildCoordinator:parentMainCoordinator];
  [parentMainCoordinator start];
  TestMainContentCoordinator* childMainCoordinator =
      [[TestMainContentCoordinator alloc] init];
  [parentMainCoordinator addChildCoordinator:childMainCoordinator];
  [childMainCoordinator start];
  EXPECT_TRUE(childMainCoordinator.viewController.UIForwarder.broadcasting);
  EXPECT_FALSE(parentMainCoordinator.viewController.UIForwarder.broadcasting);
  // Stop the child coordinator and verify that the parent's UI is being
  // broadcast instead.
  [childMainCoordinator stop];
  [parentMainCoordinator removeChildCoordinator:childMainCoordinator];
  EXPECT_TRUE(parentMainCoordinator.viewController.UIForwarder.broadcasting);
  [parentMainCoordinator stop];
  [parent() removeChildCoordinator:parentMainCoordinator];
}
