// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_tools_menu_button.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PrimaryToolbarView ()
// Container for the location bar.
@property(nonatomic, strong) UIView* locationBarContainer;

// StackView containing the leading buttons (relative to the location bar). It
// should only contains ToolbarButtons.
@property(nonatomic, strong) UIStackView* leadingStackView;
// Buttons from the leading stack view.
@property(nonatomic, strong) NSArray<ToolbarButton*>* leadingStackViewButtons;
// StackView containing the trailing buttons (relative to the location bar). It
// should only contains ToolbarButtons.
@property(nonatomic, strong) UIStackView* trailingStackView;
// Buttons from the trailing stack view.
@property(nonatomic, strong) NSArray<ToolbarButton*>* trailingStackViewButtons;

// ** Buttons in the leading stack view. **
// Button to navigate back.
@property(nonatomic, strong) ToolbarButton* backButton;
// Button to navigate forward, leading position.
@property(nonatomic, strong) ToolbarButton* forwardLeadingButton;
// Button to display the TabGrid.
@property(nonatomic, strong) ToolbarButton* tabGridButton;
// Button to stop the loading of the page.
@property(nonatomic, strong) ToolbarButton* stopButton;
// Button to reload the page.
@property(nonatomic, strong) ToolbarButton* reloadButton;

// ** Buttons in the trailing stack view. **
// Button to navigate forward, trailing position.
@property(nonatomic, strong) ToolbarButton* forwardTrailingButton;
// Button to display the share menu.
@property(nonatomic, strong) ToolbarButton* shareButton;
// Button to manage the bookmarks of this page.
@property(nonatomic, strong) ToolbarButton* bookmarkButton;
// Button to display the tools menu, redefined as readwrite.
@property(nonatomic, strong) ToolbarToolsMenuButton* toolsMenuButton;

@end

@implementation PrimaryToolbarView

@synthesize locationBarView = _locationBarView;
@synthesize topSafeAnchor = _topSafeAnchor;
@synthesize buttonFactory = _buttonFactory;
@synthesize allButtons = _allButtons;
@synthesize leadingStackView = _leadingStackView;
@synthesize leadingStackViewButtons = _leadingStackViewButtons;
@synthesize backButton = _backButton;
@synthesize forwardLeadingButton = _forwardLeadingButton;
@synthesize tabGridButton = _tabGridButton;
@synthesize stopButton = _stopButton;
@synthesize reloadButton = _reloadButton;
@synthesize locationBarContainer = _locationBarContainer;
@synthesize trailingStackView = _trailingStackView;
@synthesize trailingStackViewButtons = _trailingStackViewButtons;
@synthesize forwardTrailingButton = _forwardTrailingButton;
@synthesize shareButton = _shareButton;
@synthesize bookmarkButton = _bookmarkButton;
@synthesize toolsMenuButton = _toolsMenuButton;

#pragma mark - Setup

- (void)setUp {
  self.translatesAutoresizingMaskIntoConstraints = NO;
  self.locationBarContainer = [[UIView alloc] init];
  self.locationBarContainer.backgroundColor = [UIColor whiteColor];
  [self.locationBarContainer
      setContentHuggingPriority:UILayoutPriorityDefaultLow
                        forAxis:UILayoutConstraintAxisHorizontal];
  self.locationBarContainer.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:self.locationBarContainer];

  [self setupLeadingStackView];
  [self setupTrailingStackView];

  [self setupConstraints];
}

// Sets the leading stack view.
- (void)setupLeadingStackView {
  self.backButton = [self.buttonFactory backButton];
  self.forwardLeadingButton = [self.buttonFactory forwardButton];
  self.tabGridButton = [self.buttonFactory tabSwitcherStripButton];
  self.stopButton = [self.buttonFactory stopButton];
  self.stopButton.hiddenInCurrentState = YES;
  self.reloadButton = [self.buttonFactory reloadButton];

  self.leadingStackViewButtons = @[
    self.backButton, self.forwardLeadingButton, self.tabGridButton,
    self.stopButton, self.reloadButton
  ];
  self.leadingStackView = [[UIStackView alloc]
      initWithArrangedSubviews:self.leadingStackViewButtons];
  self.leadingStackView.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:self.leadingStackView];
}

// Sets the trailing stack view.
- (void)setupTrailingStackView {
  self.forwardTrailingButton = [self.buttonFactory forwardButton];
  self.shareButton = [self.buttonFactory shareButton];
  self.bookmarkButton = [self.buttonFactory bookmarkButton];
  self.toolsMenuButton = [self.buttonFactory toolsMenuButton];

  self.trailingStackViewButtons = @[
    self.forwardTrailingButton, self.shareButton, self.bookmarkButton,
    self.toolsMenuButton
  ];
  self.trailingStackView = [[UIStackView alloc]
      initWithArrangedSubviews:self.trailingStackViewButtons];
  self.trailingStackView.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:self.trailingStackView];
}

// Sets the constraints up.
- (void)setupConstraints {
  id<LayoutGuideProvider> safeArea = SafeAreaLayoutGuideForView(self);

  // Leading StackView constraints
  [NSLayoutConstraint activateConstraints:@[
    [self.leadingStackView.leadingAnchor
        constraintEqualToAnchor:safeArea.leadingAnchor],
    [self.leadingStackView.bottomAnchor
        constraintEqualToAnchor:safeArea.bottomAnchor],
    [self.leadingStackView.topAnchor
        constraintEqualToAnchor:self.topSafeAnchor],
  ]];

  // LocationBar constraints.
  [NSLayoutConstraint activateConstraints:@[
    [self.locationBarContainer.leadingAnchor
        constraintEqualToAnchor:self.leadingStackView.trailingAnchor],
    [self.locationBarContainer.trailingAnchor
        constraintEqualToAnchor:self.trailingStackView.leadingAnchor],
    [self.locationBarContainer.bottomAnchor
        constraintEqualToAnchor:safeArea.bottomAnchor],
    [self.locationBarContainer.topAnchor
        constraintEqualToAnchor:self.topSafeAnchor],
    [self.locationBarContainer.heightAnchor
        constraintEqualToConstant:kToolbarHeight],
  ]];

  // Trailing StackView constraints.
  [NSLayoutConstraint activateConstraints:@[
    [self.trailingStackView.trailingAnchor
        constraintEqualToAnchor:safeArea.trailingAnchor],
    [self.trailingStackView.bottomAnchor
        constraintEqualToAnchor:safeArea.bottomAnchor],
    [self.trailingStackView.topAnchor
        constraintEqualToAnchor:self.topSafeAnchor],
  ]];
}

#pragma mark - Property accessors

- (void)setLocationBarView:(UIView*)locationBarView {
  if (_locationBarView == locationBarView) {
    return;
  }
  [_locationBarView removeFromSuperview];

  locationBarView.translatesAutoresizingMaskIntoConstraints = NO;
  [locationBarView setContentHuggingPriority:UILayoutPriorityDefaultLow
                                     forAxis:UILayoutConstraintAxisHorizontal];
  [self.locationBarContainer addSubview:locationBarView];
  AddSameConstraints(self.locationBarContainer, locationBarView);
  _locationBarView = locationBarView;
}

- (NSArray<ToolbarButton*>*)allButtons {
  if (!_allButtons) {
    NSMutableArray<ToolbarButton*>* buttons = [[NSMutableArray alloc] init];
    [buttons addObjectsFromArray:self.leadingStackViewButtons];
    [buttons addObjectsFromArray:self.trailingStackViewButtons];
    _allButtons = buttons;
  }
  return _allButtons;
}

@end
