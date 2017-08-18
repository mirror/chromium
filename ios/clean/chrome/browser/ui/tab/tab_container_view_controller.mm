// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_constants.h"
#import "ios/clean/chrome/browser/ui/transitions/animators/swap_from_above_animator.h"
#import "ios/clean/chrome/browser/ui/transitions/containment_transition_context.h"
#import "ios/clean/chrome/browser/ui/transitions/containment_transitioning_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
CGFloat kToolbarHeight = 56.0f;
}

@interface TabContainerViewController ()<ContainmentTransitioningDelegate>

// Container views for child view controllers. The child view controller's
// view is added as a subview that fills its container view via autoresizing.
@property(nonatomic, strong) UIView* findBarView;
@property(nonatomic, strong) UIView* toolbarView;
@property(nonatomic, strong) UIView* contentView;

// Status Bar background view. Its size is directly linked to the difference
// between this VC's view topLayoutGuide top anchor and bottom anchor. This
// means that this view will not be displayed on landscape.
@property(nonatomic, strong) UIView* statusBarBackgroundView;

// Height constraints for toolbarView.
@property(nonatomic, strong) NSLayoutConstraint* toolbarHeightConstraint;

@end

@implementation TabContainerViewController

@synthesize contentViewController = _contentViewController;
@synthesize findBarView = _findBarView;
@synthesize findBarViewController = _findBarViewController;
@synthesize toolbarViewController = _toolbarViewController;
@synthesize toolbarView = _toolbarView;
@synthesize contentView = _contentView;
@synthesize statusBarBackgroundView = _statusBarBackgroundView;
@synthesize toolbarHeightConstraint = _toolbarHeightConstraint;
@synthesize containmentTransitioningDelegate =
    _containmentTransitioningDelegate;
@synthesize usesBottomToolbar = _usesBottomToolbar;

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  [self configureSubviews];
  [NSLayoutConstraint activateConstraints:[self subviewConstraints]];
}

#pragma mark - Public properties

- (void)setContentViewController:(UIViewController*)contentViewController {
  if (self.contentViewController == contentViewController)
    return;
  if ([self isViewLoaded]) {
    [self detachChildViewController:self.contentViewController];
    [self attachChildViewController:contentViewController
                          toSubview:self.contentView];
  }
  _contentViewController = contentViewController;
}

- (void)setFindBarViewController:(UIViewController*)findBarViewController {
  if (self.findBarViewController == findBarViewController)
    return;
  if ([self isViewLoaded]) {
    self.findBarView.hidden = NO;
    findBarViewController.view.translatesAutoresizingMaskIntoConstraints = YES;
    findBarViewController.view.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    ContainmentTransitionContext* context =
        [[ContainmentTransitionContext alloc]
            initWithFromViewController:self.findBarViewController
                      toViewController:findBarViewController
                  parentViewController:self
                                inView:self.findBarView
                            completion:^(BOOL finished) {
                              self.findBarView.hidden =
                                  (findBarViewController == nil);
                            }];
    id<UIViewControllerAnimatedTransitioning> animator =
        [self.containmentTransitioningDelegate
            animationControllerForAddingChildController:findBarViewController
                                removingChildController:
                                    self.findBarViewController
                                           toController:self];
    [context prepareTransitionWithAnimator:animator];
    [animator animateTransition:context];
  }
  _findBarViewController = findBarViewController;
}

- (void)setToolbarViewController:(UIViewController*)toolbarViewController {
  if (self.toolbarViewController == toolbarViewController)
    return;
  if ([self isViewLoaded]) {
    [self detachChildViewController:self.toolbarViewController];
    [self attachChildViewController:toolbarViewController
                          toSubview:self.toolbarView];
  }
  _toolbarViewController = toolbarViewController;
}

- (void)setUsesBottomToolbar:(BOOL)usesBottomToolbar {
  DCHECK(![self isViewLoaded]);
  _usesBottomToolbar = usesBottomToolbar;
}


#pragma mark - ChildViewController helper methods

- (void)attachChildViewController:(UIViewController*)viewController
                        toSubview:(UIView*)subview {
  if (!viewController || !subview) {
    return;
  }
  [self addChildViewController:viewController];
  viewController.view.translatesAutoresizingMaskIntoConstraints = YES;
  viewController.view.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  viewController.view.frame = subview.bounds;
  [subview addSubview:viewController.view];
  [viewController didMoveToParentViewController:self];
}

- (void)detachChildViewController:(UIViewController*)viewController {
  if (viewController.parentViewController != self)
    return;
  [viewController willMoveToParentViewController:nil];
  [viewController.view removeFromSuperview];
  [viewController removeFromParentViewController];
}

#pragma mark - MenuPresentationDelegate

- (CGRect)boundsForMenuPresentation {
  return self.view.bounds;
}
- (CGRect)originForMenuPresentation {
  return [self rectForZoomWithKey:nil inView:self.view];
}

#pragma mark - ZoomTransitionDelegate

- (CGRect)rectForZoomWithKey:(NSObject*)key inView:(UIView*)view {
  if ([self.toolbarViewController
          conformsToProtocol:@protocol(ZoomTransitionDelegate)]) {
    return [reinterpret_cast<id<ZoomTransitionDelegate>>(
        self.toolbarViewController) rectForZoomWithKey:key
                                                inView:view];
  }
  return CGRectNull;
}

#pragma mark - Methods to be overriden by subclass

- (void)configureSubviews {
  self.containmentTransitioningDelegate = self;
  self.findBarView = [[UIView alloc] init];
  self.toolbarView = [[UIView alloc] init];
  self.contentView = [[UIView alloc] init];
  self.statusBarBackgroundView = [[UIView alloc] init];
  self.findBarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.toolbarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.contentView.translatesAutoresizingMaskIntoConstraints = NO;
  self.statusBarBackgroundView.translatesAutoresizingMaskIntoConstraints = NO;
  self.view.backgroundColor = [UIColor blackColor];
  self.findBarView.backgroundColor = [UIColor clearColor];
  self.toolbarView.backgroundColor = [UIColor blackColor];
  self.contentView.backgroundColor = [UIColor blackColor];
  self.statusBarBackgroundView.backgroundColor =
      UIColorFromRGB(kToolbarBackgroundColor);
  self.findBarView.clipsToBounds = YES;

  // Views that are added last have the highest z-order.
  [self.view addSubview:self.statusBarBackgroundView];
  [self.view addSubview:self.toolbarView];
  [self.view addSubview:self.contentView];
  [self.view addSubview:self.findBarView];
  self.findBarView.hidden = YES;

  [self attachChildViewController:self.toolbarViewController
                        toSubview:self.toolbarView];
  [self attachChildViewController:self.contentViewController
                        toSubview:self.contentView];

  self.toolbarHeightConstraint =
      [self.toolbarView.heightAnchor constraintEqualToConstant:0.0f];
  self.toolbarHeightConstraint.priority = UILayoutPriorityDefaultHigh;
  if (self.toolbarViewController) {
    self.toolbarHeightConstraint.constant = kToolbarHeight;
  }
}

- (Constraints*)subviewConstraints {
  NOTREACHED() << "You must override -subviewConstraints in a subclass";
  return nil;
}

#pragma mark - ContainmentTransitioningDelegate

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForAddingChildController:(UIViewController*)addedChild
                    removingChildController:(UIViewController*)removedChild
                               toController:(UIViewController*)parent {
  return [[SwapFromAboveAnimator alloc] init];
}

@end
