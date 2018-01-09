// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/secondary_toolbar_view.h"

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_tools_menu_button.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SecondaryToolbarView ()

// The stack view containing the buttons.
@property(nonatomic, strong) UIStackView* stackView;

// Redefined as readwrite
@property(nonatomic, strong, readwrite) ToolbarToolsMenuButton* toolsMenuButton;
@property(nonatomic, strong, readwrite) ToolbarButton* tabGridButton;
@property(nonatomic, strong, readwrite) ToolbarButton* shareButton;
@property(nonatomic, strong, readwrite) ToolbarButton* omniboxButton;
@property(nonatomic, strong, readwrite) ToolbarButton* bookmarksButton;

@end

@implementation SecondaryToolbarView

@synthesize bottomSafeAnchor = _bottomSafeAnchor;
@synthesize buttonFactory = _buttonFactory;
@synthesize stackView = _stackView;
@synthesize toolsMenuButton = _toolsMenuButton;
@synthesize shareButton = _shareButton;
@synthesize omniboxButton = _omniboxButton;
@synthesize bookmarksButton = _bookmarksButton;
@synthesize tabGridButton = _tabGridButton;

#pragma mark - Setup

- (void)setUp {
  self.translatesAutoresizingMaskIntoConstraints = NO;
  self.backgroundColor = [UIColor whiteColor];

  self.tabGridButton = [self.buttonFactory tabSwitcherStripButton];
  self.shareButton = [self.buttonFactory shareButton];
  self.omniboxButton = [self.buttonFactory omniboxButton];
  self.bookmarksButton = [self.buttonFactory bookmarkButton];
  self.toolsMenuButton = [self.buttonFactory toolsMenuButton];

  self.stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
    self.tabGridButton, self.shareButton, self.omniboxButton,
    self.bookmarksButton, self.toolsMenuButton
  ]];
  self.stackView.distribution = UIStackViewDistributionEqualSpacing;
  self.stackView.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:self.stackView];

  [NSLayoutConstraint activateConstraints:@[
    [self.topAnchor constraintEqualToAnchor:self.stackView.topAnchor],
    [self.leadingAnchor constraintEqualToAnchor:self.stackView.leadingAnchor],
    [self.trailingAnchor constraintEqualToAnchor:self.stackView.trailingAnchor],
    [self.bottomSafeAnchor constraintEqualToAnchor:self.stackView.bottomAnchor],
  ]];
}

@end
