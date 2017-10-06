// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main/main_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/browser_view_controller.h"
#include "ios/chrome/browser/ui/main/main_feature_flags.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToSwitcherAnimator : NSObject<UIViewControllerAnimatedTransitioning>
@end

@implementation ToSwitcherAnimator

- (NSTimeInterval)transitionDuration:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  return 5.0;
}

- (void)animateTransition:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  UIViewController* fromViewController = [transitionContext
      viewControllerForKey:UITransitionContextFromViewControllerKey];
  UIViewController* toViewController = [transitionContext
      viewControllerForKey:UITransitionContextToViewControllerKey];

  UIView* containerView = transitionContext.containerView;

  // For a Presentation:
  //      fromView = The presenting view.
  //      toView   = The presented view.
  // For a Dismissal:
  //      fromView = The presented view.
  //      toView   = The presenting view.
  UIView* fromView =
      [transitionContext viewForKey:UITransitionContextFromViewKey];
  UIView* toView = [transitionContext viewForKey:UITransitionContextToViewKey];

  fromView.frame =
      [transitionContext initialFrameForViewController:fromViewController];
  toView.frame =
      [transitionContext finalFrameForViewController:toViewController];

  // We are responsible for adding the incoming view to the containerView
  // for the presentation/dismissal.
  [containerView addSubview:toView];

  [(id<TabSwitcher>)toViewController showWithSelectedTabAnimation:^() {
        [transitionContext completeTransition:YES];
    }];
}

@end

@interface ToBVCAnimator : NSObject<UIViewControllerAnimatedTransitioning>
@end

@implementation ToBVCAnimator

- (NSTimeInterval)transitionDuration:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  return 0.0;
}

- (void)animateTransition:
    (id<UIViewControllerContextTransitioning>)transitionContext {
  UIViewController* fromViewController = [transitionContext
      viewControllerForKey:UITransitionContextFromViewControllerKey];
  UIViewController* toViewController = [transitionContext
      viewControllerForKey:UITransitionContextToViewControllerKey];

  UIView* containerView = transitionContext.containerView;

  // For a Presentation:
  //      fromView = The presenting view.
  //      toView   = The presented view.
  // For a Dismissal:
  //      fromView = The presented view.
  //      toView   = The presenting view.
  UIView* fromView =
      [transitionContext viewForKey:UITransitionContextFromViewKey];
  UIView* toView = [transitionContext viewForKey:UITransitionContextToViewKey];

  fromView.frame =
      [transitionContext initialFrameForViewController:fromViewController];
  toView.frame =
      [transitionContext finalFrameForViewController:toViewController];

  // We are responsible for adding the incoming view to the containerView
  // for the presentation/dismissal.
  [containerView addSubview:toView];
  [transitionContext completeTransition:YES];
}

@end

@interface MainViewController ()<UIViewControllerTransitioningDelegate>
@end

@implementation MainViewController

- (UIViewController*)activeViewController {
  if (TabSwitcherPresentsBVCEnabled()) {
    UIViewController* ret = self.presentedViewController;

    if (ret.presentedViewController) {
      return ret.presentedViewController;
    } else {
      return ret;
    }
  } else {
    return [self.childViewControllers firstObject];
  }
}

- (void)setActiveViewController:(UIViewController*)activeViewController
                     completion:(void (^)())completion {
  DCHECK(activeViewController);
  if (self.activeViewController == activeViewController)
    return;

  // TODO(crbug.com/546189): DCHECK here that there isn't a modal view
  // controller showing once the known violations of that are fixed.

  if (TabSwitcherPresentsBVCEnabled()) {
    // If |activeViewController| is already on the presentation stack, dismiss
    // the current view controller and return.
    if (activeViewController ==
        self.activeViewController.presentingViewController) {
      UIViewController* presented = self.activeViewController;
      presented.transitioningDelegate = self;
      [activeViewController dismissViewControllerAnimated:YES
                                               completion:nil];
      presented.transitioningDelegate = nil;
      return;
    }

    // If the current active view controller is the stack view and the new
    // active view controller is the BVC, then present the BVC without first
    // dismissing the old view controller.
    BOOL keepOldController =
        [self.activeViewController conformsToProtocol:@protocol(TabSwitcher)] &&
        [activeViewController isKindOfClass:[BrowserViewController class]];

    if (keepOldController) {
      DCHECK(self.activeViewController);
      activeViewController.transitioningDelegate = self;
      [self.activeViewController
          presentViewController:activeViewController
                       animated:YES
                     completion:^{
                       DCHECK(self.activeViewController ==
                              activeViewController);
                       if (completion) {
                         completion();
                       }
                     }];
      activeViewController.transitioningDelegate = nil;
      return;
    }

    if (self.activeViewController) {
      // This call must be to super, as the override of
      // dismissViewControllerAnimated:completion: below tries to dismiss using
      // self.activeViewController, but this call explicitly needs to present
      // using the MainViewController itself.
      [super dismissViewControllerAnimated:NO completion:nil];
    }

    // This call must be to super, as the override of
    // presentViewController:animated:completion: below tries to present using
    // self.activeViewController, but this call explicitly needs to present
    // using the MainViewController itself.
    [super
        presentViewController:activeViewController
                     animated:NO
                   completion:^{
                     DCHECK(self.activeViewController == activeViewController);
                     if (completion) {
                       completion();
                     }
                   }];
  } else {
    // Remove the current active view controller, if any.
    if (self.activeViewController) {
      [self.activeViewController willMoveToParentViewController:nil];
      [self.activeViewController.view removeFromSuperview];
      [self.activeViewController removeFromParentViewController];
    }

    DCHECK(self.activeViewController == nil);
    DCHECK(self.view.subviews.count == 0);

    // Add the new active view controller.
    [self addChildViewController:activeViewController];
    self.activeViewController.view.frame = self.view.bounds;
    [self.view addSubview:self.activeViewController.view];
    [activeViewController didMoveToParentViewController:self];

    // Let the system know that the child has changed so appearance updates can
    // be made.
    [self setNeedsStatusBarAppearanceUpdate];

    DCHECK(self.activeViewController == activeViewController);
    if (completion) {
      completion();
    }
  }
}

#pragma mark - UIViewController methods

- (void)presentViewController:(UIViewController*)viewControllerToPresent
                     animated:(BOOL)flag
                   completion:(void (^)())completion {
  // If there is no activeViewController then this call will get inadvertently
  // dropped.
  DCHECK(self.activeViewController);
  [self.activeViewController presentViewController:viewControllerToPresent
                                          animated:flag
                                        completion:completion];
}

- (void)dismissViewControllerAnimated:(BOOL)flag
                           completion:(void (^)())completion {
  // If there is no activeViewController then this call will get inadvertently
  // dropped.
  DCHECK(self.activeViewController);
  [self.activeViewController dismissViewControllerAnimated:flag
                                                completion:completion];
}

- (UIViewController*)childViewControllerForStatusBarHidden {
  return self.activeViewController;
}

- (UIViewController*)childViewControllerForStatusBarStyle {
  return self.activeViewController;
}

- (BOOL)shouldAutorotate {
  return self.activeViewController
             ? [self.activeViewController shouldAutorotate]
             : [super shouldAutorotate];
}

#pragma mark - Transitioning Delegate

- (id<UIViewControllerAnimatedTransitioning>)animationControllerForPresentedController:(UIViewController *)presented 
                                                                  presentingController:(UIViewController *)presenting 
                                                                      sourceController:(UIViewController *)source {
  NSLog(@"Returning bvc animator");
  return [[ToBVCAnimator alloc] init];
}

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForDismissedController:(UIViewController*)dismissed {
  NSLog(@"Returning switcher animator");
  return [[ToSwitcherAnimator alloc] init];
}

@end
