// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_container_coordinator.h"

#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_container_view_controller.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabGridContainerCoordinator ()

@property(nonatomic, strong) TabGridContainerViewController* viewController;
@property(nonatomic, strong) TabGridCoordinator* normalTabGrid;

@end

@implementation TabGridContainerCoordinator

@synthesize viewController = _viewController;
@synthesize normalTabGrid = _normalTabGrid;

- (void)start {
  if (self.started)
    return;

  self.viewController = [[TabGridContainerViewController alloc] init];
  self.viewController.dispatcher = static_cast<id>(self.browser->dispatcher());
  self.normalTabGrid = [[TabGridCoordinator alloc] init];
  [self addChildCoordinator:self.normalTabGrid];
  [self.normalTabGrid start];

  [super start];
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  DCHECK(childCoordinator == self.normalTabGrid);
  self.viewController.tabGrid = childCoordinator.viewController;
}

#pragma mark - URLOpening

- (void)openURL:(NSURL*)URL {
  [self.normalTabGrid openURL:URL];
}

@end
