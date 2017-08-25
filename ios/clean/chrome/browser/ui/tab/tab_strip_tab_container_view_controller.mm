// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/tab_strip_tab_container_view_controller.h"

#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller+internal.h"
#import "ios/clean/chrome/browser/ui/ui_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Height of tabStripView when visible.
const CGFloat kTabStripOpenHeight = 120.0f;
// Height of tabStripView when not visible.
const CGFloat kTabStripClosedHeight = 0.0f;
}

@interface TabStripTabContainerViewController ()
// Containing view for a child view controller. The child view controller's view
// will completely fill this view.
@property(nonatomic, strong) UIView* tabStripView;
// Height constraint for tabStripView.
@property(nonatomic, strong) NSLayoutConstraint* tabStripHeightConstraint;
@end

@implementation TabStripTabContainerViewController
@synthesize tabStripViewController = _tabStripViewController;
@synthesize tabStripVisible = _tabStripVisible;
@synthesize tabStripView = _tabStripView;
@synthesize tabStripHeightConstraint = _tabStripHeightConstraint;

#pragma mark - Public properties

- (void)setTabStripViewController:(UIViewController*)tabStripViewController {
  if (_tabStripViewController == tabStripViewController)
    return;
  if ([self isViewLoaded]) {
    [self detachChildViewController:_tabStripViewController];
    [self attachChildViewController:tabStripViewController
                          toSubview:self.tabStripView];
  }
  _tabStripViewController = tabStripViewController;
}

- (void)setTabStripVisible:(BOOL)tabStripVisible {
  self.tabStripHeightConstraint.constant =
      tabStripVisible ? kTabStripOpenHeight : kTabStripClosedHeight;
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
  [self attachChildViewController:self.tabStripViewController
                        toSubview:self.tabStripView];
}

- (Constraints*)subviewConstraints {
  return self.usesBottomToolbar ? [self bottomToolbarConstraints]
                                : [self topToolbarConstraints];
}

#pragma mark - Private methods

// Constraints that place the toolbar at the top.
- (Constraints*)topToolbarConstraints {
  NSMutableArray* constraints =
      [NSMutableArray arrayWithArray:[self commonConstraints]];
  [constraints addObjectsFromArray:[self uniqueTopToolbarConstraints]];
  return constraints;
}

// Constraints that place the toolbar at the bottom.
- (Constraints*)bottomToolbarConstraints {
  NSMutableArray* constraints =
      [NSMutableArray arrayWithArray:[self commonConstraints]];
  [constraints addObjectsFromArray:[self uniqueBottomToolbarConstraints]];
  return constraints;
}

// Constraints that are shared between topToolbar and bottomToolbar
// configurations.
- (Constraints*)commonConstraints {
  self.tabStripHeightConstraint = [self.tabStripView.heightAnchor
      constraintEqualToConstant:kTabStripClosedHeight];
  return @[
    // StatusBarBackground constraints.
    [self.statusBarBackgroundView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.topAnchor],
    [self.statusBarBackgroundView.bottomAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.statusBarBackgroundView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.statusBarBackgroundView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    // TabStrip constraints.
    [self.tabStripView.topAnchor
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.tabStripView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.tabStripView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    self.tabStripHeightConstraint,

    // Findbar constraints.
    [self.findBarView.topAnchor
        constraintEqualToAnchor:self.toolbarView.topAnchor],
    [self.findBarView.bottomAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.findBarView.leadingAnchor
        constraintEqualToAnchor:self.toolbarView.leadingAnchor],
    [self.findBarView.trailingAnchor
        constraintEqualToAnchor:self.toolbarView.trailingAnchor],

    // Toolbar leading, trailing, and height constraints.
    [self.toolbarView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.toolbarView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
    [self.toolbarView.heightAnchor constraintEqualToConstant:kToolbarHeight],

    // Content leading and trailing constraints.
    [self.contentView.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor],
    [self.contentView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],
  ];
}

// These constraints uniquely place the toolbar at the top.
- (Constraints*)uniqueTopToolbarConstraints {
  return @[
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.tabStripView.bottomAnchor],
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.toolbarView.bottomAnchor],
    [self.contentView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],
  ];
}

// These constraints uniquely place the toolbar at the bottom.
- (Constraints*)uniqueBottomToolbarConstraints {
  return @[
    [self.contentView.topAnchor
        constraintEqualToAnchor:self.tabStripView.bottomAnchor],
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor],
    [self.toolbarView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],
  ];
}

#pragma mark - Tab Strip actions.

- (void)hideTabStrip:(id)sender {
  self.tabStripVisible = NO;
}

@end
