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
#import "ios/clean/chrome/browser/ui/ui_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
CGFloat kToolbarHeight = 56.0f;
CGFloat kTabStripHeight = 120.0f;
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

// Configures all the subviews. Subclasses may override this method to configure
// additional subviews. Subclass overrides must first call the super method.
- (void)configureSubviews;

// Returns all the constraints required for this view controller. Subclasses
// must override this method.
- (Constraints*)subviewConstraints;

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
    [self addChildViewController:contentViewController
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
    [self addChildViewController:toolbarViewController
                       toSubview:self.toolbarView];
  }
  _toolbarViewController = toolbarViewController;
}

- (void)setUsesBottomToolbar:(BOOL)usesBottomToolbar {
  DCHECK(![self isViewLoaded]);
  _usesBottomToolbar = usesBottomToolbar;
}


#pragma mark - ChildViewController helper methods

- (void)addChildViewController:(UIViewController*)viewController
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

  [self addChildViewController:self.toolbarViewController
                     toSubview:self.toolbarView];
  [self addChildViewController:self.contentViewController
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

@implementation BasicTabContainerViewController

#pragma mark - TabContainerViewController overrides

- (Constraints*)subviewConstraints {
  if (self.usesBottomToolbar) {
    return [self bottomToolbarConstraints];
  }
  return [self topToolbarConstraints];
}

#pragma mark - Private methods

- (Constraints*)topToolbarConstraints {
  return @[
    [self.statusBarBackgroundView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.statusBarBackgroundView.bottomAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.statusBarBackgroundView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.statusBarBackgroundView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.toolbarHeightConstraint,
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.contentView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.contentView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [self.contentView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],

    [self.findBarView.topAnchor
        constraintEqualToAnchor:self.toolbarView.topAnchor],
    [self.findBarView.bottomAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.findBarView.leadingAnchor
        constraintEqualToAnchor:self.toolbarView.leadingAnchor],
    [self.findBarView.trailingAnchor
        constraintEqualToAnchor:self.toolbarView.trailingAnchor],
  ];
}

- (Constraints*)bottomToolbarConstraints {
  return @[
    [self.statusBarBackgroundView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.statusBarBackgroundView.bottomAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.statusBarBackgroundView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.statusBarBackgroundView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    [self.contentView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.contentView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.contentView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor],
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.toolbarHeightConstraint,
    [self.toolbarView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],
  ];
}

@end

@interface TabStripTabContainerViewController ()
@property(nonatomic, strong) UIView* tabStripView;
// Height constraints for tabStripView and toolbarView.
@property(nonatomic, strong) NSLayoutConstraint* tabStripHeightConstraint;
@end

@implementation TabStripTabContainerViewController
@synthesize tabStripViewController = _tabStripViewController;
@synthesize tabStripVisible = _tabStripVisible;
@synthesize tabStripView = _tabStripView;
@synthesize tabStripHeightConstraint = _tabStripHeightConstraint;

#pragma mark - Public properties

- (void)setTabStripViewController:(UIViewController*)tabStripViewController {
  if (self.tabStripViewController == tabStripViewController)
    return;
  if ([self isViewLoaded]) {
    [self detachChildViewController:self.tabStripViewController];
    [self addChildViewController:tabStripViewController
                       toSubview:self.tabStripView];
  }
  _tabStripViewController = tabStripViewController;
}

- (void)setTabStripVisible:(BOOL)tabStripVisible {
  if (tabStripVisible) {
    self.tabStripHeightConstraint.constant = kTabStripHeight;
  } else {
    self.tabStripHeightConstraint.constant = 0.0f;
  }
  _tabStripVisible = tabStripVisible;
}

#pragma mark - TabContainerViewController overrides

- (void)configureSubviews {
  [super configureSubviews];
  self.tabStripView = [[UIView alloc] init];
  self.tabStripView.translatesAutoresizingMaskIntoConstraints = NO;
  self.tabStripView.backgroundColor = [UIColor blackColor];
  // Insert below other views inserted in the super class.
  [self.view insertSubview:self.tabStripView atIndex:0];
  [self addChildViewController:self.tabStripViewController
                     toSubview:self.tabStripView];
  self.tabStripHeightConstraint =
      [self.tabStripView.heightAnchor constraintEqualToConstant:0.0f];
}

- (Constraints*)subviewConstraints {
  if (self.usesBottomToolbar) {
    return [self bottomToolbarConstraints];
  }
  return [self topToolbarConstraints];
}

#pragma mark - Private methods

- (Constraints*)topToolbarConstraints {
  return @[
    [self.statusBarBackgroundView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.statusBarBackgroundView.bottomAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.statusBarBackgroundView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.statusBarBackgroundView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    [self.tabStripView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.tabStripView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.tabStripView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.tabStripHeightConstraint,
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.tabStripView.bottomAnchor],
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.toolbarHeightConstraint,
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.contentView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.contentView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [self.contentView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],

    [self.findBarView.topAnchor
        constraintEqualToAnchor:self.toolbarView.topAnchor],
    [self.findBarView.bottomAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.findBarView.leadingAnchor
        constraintEqualToAnchor:self.toolbarView.leadingAnchor],
    [self.findBarView.trailingAnchor
        constraintEqualToAnchor:self.toolbarView.trailingAnchor],
  ];
}

- (Constraints*)bottomToolbarConstraints {
  return @[
    [self.statusBarBackgroundView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.statusBarBackgroundView.bottomAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.statusBarBackgroundView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.statusBarBackgroundView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    [self.tabStripView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.tabStripView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.tabStripView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.tabStripHeightConstraint,
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.tabStripView.bottomAnchor],
    [self.contentView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.contentView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor],
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.toolbarHeightConstraint,
    [self.toolbarView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],
  ];
}

#pragma mark - Tab Strip actions.

- (void)hideTabStrip:(id)sender {
  self.tabStripVisible = NO;
}

@end
