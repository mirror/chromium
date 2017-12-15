// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/chrome_coordinator.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ChromeCoordinator ()

// Helper function used in initializers.
- (void)commonInitForBaseViewController:(UIViewController*)baseViewController
                           browserState:(ios::ChromeBrowserState*)browserState;

@end

@implementation ChromeCoordinator
@synthesize childCoordinators = _childCoordinators;
@synthesize baseViewController = _baseViewController;
@synthesize browserState = _browserState;

- (nullable instancetype)initWithBaseViewController:
    (UIViewController*)viewController {
  if (self = [super init]) {
    [self commonInitForBaseViewController:viewController browserState:nullptr];
  }
  return self;
}

- (nullable instancetype)
initWithBaseViewController:(UIViewController*)viewController
              browserState:(ios::ChromeBrowserState*)browserState {
  if (self = [super init]) {
    [self commonInitForBaseViewController:viewController
                             browserState:browserState];
  }
  return self;
}

- (void)dealloc {
  [self stop];
}

#pragma mark - Accessors

- (ChromeCoordinator*)activeChildCoordinator {
  // By default the active child is the one most recently added to the child
  // array, but subclasses can override this.
  return self.childCoordinators.lastObject;
}

#pragma mark - Public

- (void)start {
  // Default implementation does nothing.
}

- (void)stop {
  // Default implementation does nothing.
}

#pragma mark - Private

- (void)commonInitForBaseViewController:(UIViewController*)baseViewController
                           browserState:(ios::ChromeBrowserState*)browserState {
  _baseViewController = baseViewController;
  _childCoordinators = [MutableCoordinatorArray array];
  _browserState = browserState;
}

@end
