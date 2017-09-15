// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller.h"
#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller+internal.h"

#include "base/i18n/rtl.h"
#import "base/logging.h"
#import "ios/chrome/browser/ui/UIView+SizeClassSupport.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/clean/chrome/browser/ui/guides/guides.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_constants.h"
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

// Status Bar background view. Its size is directly linked to the difference
// between this VC's view topLayoutGuide top anchor and bottom anchor. This
// means that this view will not be displayed on landscape.
@property(nonatomic, strong) UIView* statusBarBackgroundView;

// Handy references
@property(nonatomic, weak) UILayoutGuide* omniboxGuide;

// Constraints used to horizontally position the findbar on tablet.  Exactly one
// of these sets of constraints will be active at any given time.  Callers
// should deactivate the old set before activating the new set, because a given
// constraint may be in more than one set.  These properties are nil on phone.
//
// Positions the findbar to match the omnibox.
@property(nonatomic, readwrite, strong) Constraints* findBarOmniboxConstraints;
// Positions the findbar to be the full width of the toolbar.
@property(nonatomic, readwrite, strong) Constraints* findBarToolbarConstraints;
// Positions the findbar to be right-aligned with the omnibox and a set width.
@property(nonatomic, readwrite, strong) Constraints* findBarSetWidthConstraints;

@end

@implementation TabContainerViewController
@synthesize contentViewController = _contentViewController;
@synthesize findBarViewController = _findBarViewController;
@synthesize toolbarViewController = _toolbarViewController;
@synthesize containerView = _containerView;
@synthesize findBarView = _findBarView;
@synthesize toolbarView = _toolbarView;
@synthesize contentView = _contentView;
@synthesize statusBarBackgroundView = _statusBarBackgroundView;
@synthesize containmentTransitioningDelegate =
    _containmentTransitioningDelegate;
@synthesize usesBottomToolbar = _usesBottomToolbar;
@synthesize statusBarBackgroundColor = _statusBarBackgroundColor;
@synthesize omniboxGuide = _omniboxGuide;
@synthesize findBarOmniboxConstraints = _findBarOmniboxConstraints;
@synthesize findBarToolbarConstraints = _findBarToolbarConstraints;
@synthesize findBarSetWidthConstraints = _findBarSetWidthConstraints;

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  self.omniboxGuide = AddNamedGuide(kOmniboxGuide, self.view);

  [self configureSubviews];

  NSMutableArray* constraints = [NSMutableArray array];
  [constraints addObjectsFromArray:[self statusBarBackgroundConstraints]];
  [constraints addObjectsFromArray:[self commonToolbarConstraints]];
  [constraints addObjectsFromArray:(self.usesBottomToolbar
                                        ? [self bottomToolbarConstraints]
                                        : [self topToolbarConstraints])];
  [constraints addObjectsFromArray:[self findbarConstraints]];
  [constraints addObjectsFromArray:[self containerConstraints]];
  [NSLayoutConstraint activateConstraints:constraints];
  [self updateFindBarConstraints];
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
    [self detachChildViewController:self.findBarViewController];
    if (findBarViewController) {
      [self addChildViewController:findBarViewController];
      [self.findBarView addSubview:findBarViewController.view];

      findBarViewController.view.translatesAutoresizingMaskIntoConstraints = NO;
      [NSLayoutConstraint activateConstraints:@[
        [self.findBarView.leadingAnchor
            constraintEqualToAnchor:findBarViewController.view.leadingAnchor],
        [self.findBarView.trailingAnchor
            constraintEqualToAnchor:findBarViewController.view.trailingAnchor],
        [self.findBarView.topAnchor
            constraintEqualToAnchor:findBarViewController.view.topAnchor],
        [self.findBarView.bottomAnchor
            constraintEqualToAnchor:findBarViewController.view.bottomAnchor],
      ]];

      [findBarViewController didMoveToParentViewController:self];
    }
  }
  _findBarViewController = findBarViewController;
  self.findBarView.hidden = (_findBarViewController == nil);
  [self updateFindBarConstraints];
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

- (void)setStatusBarBackgroundColor:(UIColor*)statusBarBackgroundColor {
  _statusBarBackgroundColor = statusBarBackgroundColor;
  self.statusBarBackgroundView.backgroundColor = statusBarBackgroundColor;
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
  self.statusBarBackgroundView = [[UIView alloc] init];
  self.containerView.translatesAutoresizingMaskIntoConstraints = NO;
  self.findBarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.toolbarView.translatesAutoresizingMaskIntoConstraints = NO;
  self.contentView.translatesAutoresizingMaskIntoConstraints = NO;
  self.statusBarBackgroundView.translatesAutoresizingMaskIntoConstraints = NO;
  self.view.backgroundColor = [UIColor blackColor];
  self.findBarView.backgroundColor = [UIColor clearColor];
  self.toolbarView.backgroundColor = [UIColor blackColor];
  self.contentView.backgroundColor = [UIColor blackColor];
  self.statusBarBackgroundView.backgroundColor = self.statusBarBackgroundColor;
  self.findBarView.clipsToBounds = YES;

  [self.view addSubview:self.containerView];
  [self.view addSubview:self.statusBarBackgroundView];
  [self.containerView addSubview:self.toolbarView];
  [self.containerView addSubview:self.contentView];
  // Findbar should have higher z-order than toolbar.
  [self.containerView addSubview:self.findBarView];
  self.findBarView.hidden = YES;

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

// Constraints for the status bar background.
- (Constraints*)statusBarBackgroundConstraints {
  return @[
    [self.statusBarBackgroundView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.statusBarBackgroundView.bottomAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.statusBarBackgroundView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.statusBarBackgroundView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
  ];
}

// Constraints for the findbar.
- (Constraints*)findbarConstraints {
  if (IsIPadIdiom()) {
    // The amount of transparency on the leading and trailing edges of the find
    // bar background image.
    const CGFloat findBarPadding = 6.0;
    const CGFloat findBarWidth = 345.0;

    // This constraint is shared across multiple sets below.
    NSLayoutConstraint* omniboxTrailingConstraint =
        [self.findBarView.trailingAnchor
            constraintEqualToAnchor:self.omniboxGuide.trailingAnchor
                           constant:findBarPadding];

    self.findBarOmniboxConstraints = @[
      [self.findBarView.leadingAnchor
          constraintEqualToAnchor:self.omniboxGuide.leadingAnchor
                         constant:-findBarPadding],
      omniboxTrailingConstraint,
    ];

    self.findBarSetWidthConstraints = @[
      [self.findBarView.widthAnchor constraintEqualToConstant:findBarWidth],
      omniboxTrailingConstraint,
    ];

    self.findBarToolbarConstraints = @[
      [self.findBarView.leadingAnchor
          constraintEqualToAnchor:self.toolbarView.leadingAnchor],
      [self.findBarView.trailingAnchor
          constraintEqualToAnchor:self.toolbarView.trailingAnchor],
    ];

    // On tablet, the findbar is positioned underneath the toolbar, with a
    // slight overlap.  The height is intrinsic to the findbarview and is not
    // overridden here.
    return @[
      [self.findBarView.topAnchor
          constraintEqualToAnchor:self.toolbarView.bottomAnchor
                         constant:-10],
    ];
  } else {
    // On phone, the findbar shares a frame with the toolbar.
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
}

// Constraints that are shared between topToolbar and bottomToolbar
// configurations.
- (Constraints*)commonToolbarConstraints {
  return @[
    // Toolbar leading, trailing, and height constraints.
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.containerView.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.containerView.trailingAnchor],
    [self.toolbarView.heightAnchor constraintEqualToConstant:kToolbarHeight],

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
        constraintEqualToAnchor:self.containerView.topAnchor],
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.contentView.bottomAnchor
        constraintEqualToAnchor:self.containerView.bottomAnchor],
  ];
}

// Constraints that configure the toolbar at the bottom.
- (Constraints*)bottomToolbarConstraints {
  return @[
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.containerView.topAnchor],
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor],
    [self.toolbarView.bottomAnchor
        constraintEqualToAnchor:self.containerView.bottomAnchor],
  ];
}

- (void)updateFindBarConstraints {
  if (!IsIPadIdiom()) {
    return;
  }

  CGFloat prev = CGRectGetWidth(self.omniboxGuide.layoutFrame);
  [self.view layoutIfNeeded];
  NSLog(@"%f -> %f", prev, CGRectGetWidth(self.omniboxGuide.layoutFrame));

  // On iPad, there are three possible frames for the Search bar:
  if (self.view.cr_widthSizeClass == REGULAR) {
    // 1. In Regular width size class, it is short, right-aligned to the
    //    omnibox's right edge.
    [NSLayoutConstraint deactivateConstraints:self.findBarOmniboxConstraints];
    [NSLayoutConstraint deactivateConstraints:self.findBarToolbarConstraints];
    [NSLayoutConstraint activateConstraints:self.findBarSetWidthConstraints];
  } else {
    const CGFloat minWidth = 345.0;
    if (CGRectGetWidth(self.omniboxGuide.layoutFrame) >= minWidth) {
      // 2. In Compact size class, if the short bar width is less than the
      //    omnibox, stretch and align the search bar to the omnibox.
      [NSLayoutConstraint
          deactivateConstraints:self.findBarSetWidthConstraints];
      [NSLayoutConstraint deactivateConstraints:self.findBarToolbarConstraints];
      [NSLayoutConstraint activateConstraints:self.findBarOmniboxConstraints];
    } else {
      // 3. Finally, if the short bar width is more than the omnibox, fill the
      //    container view from edge to edge, ignoring the omnibox.
      [NSLayoutConstraint
          deactivateConstraints:self.findBarSetWidthConstraints];
      [NSLayoutConstraint deactivateConstraints:self.findBarOmniboxConstraints];
      [NSLayoutConstraint activateConstraints:self.findBarToolbarConstraints];
    }
  }

  [self.view setNeedsLayout];
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  [self updateFindBarConstraints];
}

@end
