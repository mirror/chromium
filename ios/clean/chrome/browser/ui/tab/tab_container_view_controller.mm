// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller.h"
#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller+internal.h"

#import "base/logging.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"
#import "ios/clean/chrome/browser/ui/transitions/animators/swap_from_above_animator.h"
#import "ios/clean/chrome/browser/ui/transitions/containment_transition_context.h"
#import "ios/clean/chrome/browser/ui/transitions/containment_transitioning_delegate.h"
#import "ios/clean/chrome/browser/ui/ui_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kToolbarHeight = 56.0f;
}  // namespace

@interface TabContainerViewController ()<ContainmentTransitioningDelegate>

// Container view enclosing all child view controllers.
@property(nonatomic, strong) UIView* containerView;

// Container views for child view controllers. The child view controller's
// view is added as a subview that fills its container view via autoresizing.
@property(nonatomic, strong) UIView* findBarView;
@property(nonatomic, strong) UIView* toolbarView;
@property(nonatomic, strong) UIView* contentView;

// A layout guide representing the space visually occupied by the toolbar.  When
// fullscreen animations occur, it is translated up and down using its transform
// and this layout guide updates to represent the visual space occupied by the
// transformed layer.
@property(nonatomic, strong) UILayoutGuide* visibleToolbarLayoutGuide;
@property(nonatomic, strong) NSLayoutConstraint* visibleToolbarHeightConstraint;

// Returns the vertical translation of the toolbar from its fully visible
// position for a fullscreen progress value.
- (CGFloat)toolbarDeltaForFullscreenProgress:(CGFloat)progress;
// Returns the transform that translates the toolbar for fullscreen events.
- (CGAffineTransform)toolbarTransformForFullscreenProgress:(CGFloat)progress;

@end

@implementation TabContainerViewController
@synthesize contentViewController = _contentViewController;
@synthesize findBarViewController = _findBarViewController;
@synthesize toolbarViewController = _toolbarViewController;
@synthesize containerView = _containerView;
@synthesize findBarView = _findBarView;
@synthesize toolbarView = _toolbarView;
@synthesize contentView = _contentView;
@synthesize containmentTransitioningDelegate =
    _containmentTransitioningDelegate;
@synthesize usesBottomToolbar = _usesBottomToolbar;
@synthesize visibleToolbarLayoutGuide = _visibleToolbarLayoutGuide;
@synthesize visibleToolbarHeightConstraint = _visibleToolbarHeightConstraint;

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];
  [self configureSubviews];

  NSMutableArray* constraints = [NSMutableArray array];
  [constraints addObjectsFromArray:[self commonToolbarConstraints]];
  [constraints addObjectsFromArray:(self.usesBottomToolbar
                                        ? [self bottomToolbarConstraints]
                                        : [self topToolbarConstraints])];
  [constraints addObjectsFromArray:[self findbarConstraints]];
  [constraints addObjectsFromArray:[self containerConstraints]];
  [NSLayoutConstraint activateConstraints:constraints];
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

- (CGFloat)toolbarHeight {
  return kToolbarHeight;
}

#pragma mark - FullscreenUIElement

- (void)updateForFullscreenProgress:(CGFloat)progress {
  self.toolbarView.transform =
      [self toolbarTransformForFullscreenProgress:progress];
  self.visibleToolbarHeightConstraint.constant =
      [self toolbarDeltaForFullscreenProgress:progress];
}

- (void)updateForFullscreenEnabled:(BOOL)enabled {
  self.toolbarView.transform = CGAffineTransformIdentity;
  self.visibleToolbarHeightConstraint.constant =
      CGRectGetHeight(self.toolbarView.bounds);
}

- (void)finishFullscreenScrollWithAnimator:
    (FullscreenScrollEndAnimator*)animator {
  CGFloat finalProgress = animator.finalProgress;
  [animator addAnimations:^{
    self.toolbarView.transform =
        [self toolbarTransformForFullscreenProgress:finalProgress];
    // Calculate the final frame of the content view since adjusting the visible
    // toolbar height constraint is not not animatable.
    CGFloat verticalInset =
        kToolbarHeight + [self toolbarDeltaForFullscreenProgress:finalProgress];
    UIEdgeInsets insets = self.usesBottomToolbar
                              ? UIEdgeInsetsMake(0, 0, verticalInset, 0)
                              : UIEdgeInsetsMake(verticalInset, 0, 0, 0);
    self.contentView.frame =
        UIEdgeInsetsInsetRect(self.containerView.bounds, insets);
  }];
  [animator addCompletion:^(UIViewAnimatingPosition finalPosition) {
    self.visibleToolbarHeightConstraint.constant =
        [self toolbarDeltaForFullscreenProgress:finalProgress];
  }];
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

#pragma mark - Methods in Internal category

- (void)attachChildViewController:(UIViewController*)viewController
                        toSubview:(UIView*)subview {
  if (!viewController || !subview)
    return;
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

- (void)configureSubviews {
  self.containmentTransitioningDelegate = self;
  self.containerView = [[UIView alloc] init];
  self.findBarView = [[UIView alloc] init];
  self.toolbarView = [[UIView alloc] init];
  self.contentView = [[UIView alloc] init];
  self.containerView.translatesAutoresizingMaskIntoConstraints = NO;
  self.findBarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.toolbarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.contentView.translatesAutoresizingMaskIntoConstraints = NO;
  self.view.backgroundColor = [UIColor blackColor];
  self.findBarView.backgroundColor = [UIColor clearColor];
  self.toolbarView.backgroundColor = [UIColor blackColor];
  self.contentView.backgroundColor = [UIColor blackColor];
  self.findBarView.clipsToBounds = YES;

  [self.view addSubview:self.containerView];
  [self.containerView addSubview:self.toolbarView];
  [self.containerView addSubview:self.contentView];
  // Findbar should have higher z-order than toolbar.
  [self.containerView addSubview:self.findBarView];
  self.findBarView.hidden = YES;

  self.visibleToolbarLayoutGuide = [[UILayoutGuide alloc] init];
  [self.containerView addLayoutGuide:self.visibleToolbarLayoutGuide];

  [self attachChildViewController:self.toolbarViewController
                        toSubview:self.toolbarView];
  [self attachChildViewController:self.contentViewController
                        toSubview:self.contentView];
}

- (Constraints*)containerConstraints {
  return @[
    [self.containerView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.containerView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],
    [self.containerView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.containerView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
  ];
}

#pragma mark - ContainmentTransitioningDelegate

- (id<UIViewControllerAnimatedTransitioning>)
animationControllerForAddingChildController:(UIViewController*)addedChild
                    removingChildController:(UIViewController*)removedChild
                               toController:(UIViewController*)parent {
  return [[SwapFromAboveAnimator alloc] init];
}

#pragma mark - Private methods

// Constraints for the findbar.
- (Constraints*)findbarConstraints {
  return @[
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

// Constraints that are shared between topToolbar and bottomToolbar
// configurations.
- (Constraints*)commonToolbarConstraints {
  self.visibleToolbarHeightConstraint =
      [self.visibleToolbarLayoutGuide.heightAnchor
          constraintEqualToAnchor:self.toolbarView.heightAnchor
                       multiplier:1.0
                         constant:0.0];
  return @[
    // Toolbar leading, trailing constraints.
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.containerView.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.containerView.trailingAnchor],

    // The visible toolbar guide leading and trailing cosntraints are
    // constrained to the toolbar view.
    [self.visibleToolbarLayoutGuide.leadingAnchor
        constraintEqualToAnchor:self.toolbarView.leadingAnchor],
    [self.visibleToolbarLayoutGuide.trailingAnchor
        constraintEqualToAnchor:self.toolbarView.trailingAnchor],
    self.visibleToolbarHeightConstraint,

    // Content leading and trailing constraints.
    [self.contentView.leadingAnchor
        constraintEqualToAnchor:self.containerView.leadingAnchor],
    [self.contentView.trailingAnchor
        constraintEqualToAnchor:self.containerView.trailingAnchor],
  ];
}

// Constraints that configure the toolbar at the top.
- (Constraints*)topToolbarConstraints {
  return @[
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.visibleToolbarLayoutGuide.bottomAnchor],
    [self.contentView.bottomAnchor
        constraintEqualToAnchor:self.containerView.bottomAnchor],
    [self.toolbarView.heightAnchor
        constraintEqualToConstant:kToolbarHeight +
                                  CGRectGetHeight(
                                      [UIApplication sharedApplication]
                                          .statusBarFrame)],
    [self.visibleToolbarLayoutGuide.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
  ];
}

// Constraints that configure the toolbar at the bottom.
- (Constraints*)bottomToolbarConstraints {
  return @[
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.containerView.topAnchor],
    [self.contentView.bottomAnchor
        constraintEqualToAnchor:self.visibleToolbarLayoutGuide.topAnchor],
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor],
    [self.toolbarView.bottomAnchor
        constraintEqualToAnchor:self.containerView.bottomAnchor],
    [self.toolbarView.heightAnchor constraintEqualToConstant:kToolbarHeight],
    [self.visibleToolbarLayoutGuide.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
  ];
}

- (CGFloat)toolbarDeltaForFullscreenProgress:(CGFloat)progress {
  return (progress - 1.0) * self.toolbarHeight;
}

- (CGAffineTransform)toolbarTransformForFullscreenProgress:(CGFloat)progress {
  return CGAffineTransformMakeTranslation(
      0.0, [self toolbarDeltaForFullscreenProgress:progress]);
}

@end
