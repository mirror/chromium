// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/root/root_container_view_controller.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface RootContainerViewController ()
@property(nonatomic, strong) UIViewController* launchScreenController;
@end

@implementation RootContainerViewController

@synthesize contentViewController = _contentViewController;
@synthesize launchScreenController = _launchScreenController;

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = [UIColor blackColor];
  [self attachChildViewController:self.contentViewController];

  // Load view from Launch Screen and add it to window.
  // It will be displayed until |self.contentViewController| is ready.
  NSBundle* mainBundle = [NSBundle mainBundle];
  NSArray* topObjects =
      [mainBundle loadNibNamed:@"LaunchScreen" owner:self options:nil];
  self.launchScreenController = [topObjects lastObject];
  [self.view addSubview:self.launchScreenController.view];
  [self.view bringSubviewToFront:self.launchScreenController.view];
}

#pragma mark - Public properties

- (void)setContentViewController:(UIViewController*)contentViewController {
  if (self.contentViewController == contentViewController)
    return;
  if ([self isViewLoaded]) {
    [self detachChildViewController:self.contentViewController];
    [self attachChildViewController:contentViewController];
  }
  _contentViewController = contentViewController;
  if (self.launchScreenController) {
    // If hideSplashScreen has not been called yet, keep the new view hidden.
    // This allows to prepare it before displaying it.
    self.contentViewController.view.hidden = true;
  }
}

- (void)hideSplashScreen {
  [self.launchScreenController.view removeFromSuperview];
  self.launchScreenController = nil;
  self.contentViewController.view.hidden = false;
}

#pragma mark - ChildViewController helper methods

- (void)attachChildViewController:(UIViewController*)viewController {
  if (!viewController) {
    return;
  }
  [self addChildViewController:viewController];
  viewController.view.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  viewController.view.frame = self.view.bounds;
  [self.view addSubview:viewController.view];
  [viewController didMoveToParentViewController:self];
}

- (void)detachChildViewController:(UIViewController*)viewController {
  if (!viewController) {
    return;
  }
  DCHECK_EQ(self, viewController.parentViewController);
  [viewController willMoveToParentViewController:nil];
  [viewController.view removeFromSuperview];
  [viewController removeFromParentViewController];
}

#pragma mark - ZoomTransitionDelegate

- (CGRect)rectForZoomWithKey:(NSObject*)key inView:(UIView*)view {
  if ([self.contentViewController
          conformsToProtocol:@protocol(ZoomTransitionDelegate)]) {
    return [static_cast<id<ZoomTransitionDelegate>>(self.contentViewController)
        rectForZoomWithKey:key
                    inView:view];
  }
  return CGRectNull;
}

#pragma mark - MenuPresentationDelegate

- (CGRect)boundsForMenuPresentation {
  return self.view.bounds;
}
- (CGRect)originForMenuPresentation {
  return [self rectForZoomWithKey:nil inView:self.view];
}

@end
