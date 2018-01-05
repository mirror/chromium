// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PrimaryToolbarView ()
@property(nonatomic, strong) UIStackView* stackView;
@property(nonatomic, strong) UIView* locationBarContainer;
@end

@implementation PrimaryToolbarView

@synthesize buttonFactory = _buttonFactory;
@synthesize locationBarView = _locationBarView;
@synthesize stackView = _stackView;
@synthesize locationBarContainer = _locationBarContainer;
@synthesize topSafeAnchor = _topSafeAnchor;

#pragma mark - Setup

- (void)setUp {
  self.locationBarContainer = [[UIView alloc] init];
  self.locationBarContainer.backgroundColor = [UIColor whiteColor];
  self.stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
    [self.buttonFactory tabSwitcherStripButton], self.locationBarContainer
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

@end
