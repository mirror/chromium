// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/basic_tab_container_view_controller.h"

#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller+internal.h"
#import "ios/clean/chrome/browser/ui/ui_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BasicTabContainerViewController

#pragma mark - TabContainerViewController overrides

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
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
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
        constraintEqualToAnchor:self.topLayoutGuide.bottomAnchor],
    [self.toolbarView.topAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor],
    [self.toolbarView.bottomAnchor
        constraintEqualToAnchor:self.bottomLayoutGuide.topAnchor],
  ];
}

@end
