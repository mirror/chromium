// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/host_fetching_error_view_controller.h"

#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MDCTypography.h"
#import "remoting/ios/app/remoting_theme.h"

static const CGFloat kPadding = 20;
static const CGFloat kButtonRightPaddingAdjustment = -10;
static const CGFloat kButtonBottomPaddingAdjustment = -5;
static const CGFloat kLineSpace = 30;

@implementation HostFetchingErrorViewController {
  UILabel* _label;
}

@synthesize onRetryCallback = _onRetryCallback;

- (instancetype)init {
  if (self = [super init]) {
    _label = [[UILabel alloc] initWithFrame:CGRectZero];
  }
  return self;
}

- (void)viewDidLoad {
  UIView* contentView = [[UIView alloc] initWithFrame:CGRectZero];
  contentView.backgroundColor = RemotingTheme.setupListBackgroundColor;
  contentView.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:contentView];

  _label.font = MDCTypography.body1Font;
  _label.numberOfLines = 0;
  _label.lineBreakMode = NSLineBreakByWordWrapping;
  _label.textColor = RemotingTheme.setupListTextColor;
  _label.translatesAutoresizingMaskIntoConstraints = NO;
  [contentView addSubview:_label];

  MDCButton* button = [[MDCButton alloc] initWithFrame:CGRectZero];
  [button setTitle:@"Retry" forState:UIControlStateNormal];
  [button setBackgroundColor:UIColor.clearColor forState:UIControlStateNormal];
  [button setTitleColor:RemotingTheme.flatButtonTextColor
               forState:UIControlStateNormal];
  [button sizeToFit];
  [button addTarget:self
                action:@selector(didTapRetry:)
      forControlEvents:UIControlEventTouchUpInside];
  button.translatesAutoresizingMaskIntoConstraints = NO;
  [contentView addSubview:button];

  [NSLayoutConstraint activateConstraints:@[
    [contentView.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    [contentView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
    [contentView.trailingAnchor
        constraintEqualToAnchor:self.view.trailingAnchor],

    [_label.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor
                                         constant:kPadding],
    [_label.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor
                                          constant:-kPadding],
    [_label.topAnchor constraintEqualToAnchor:contentView.topAnchor
                                     constant:kPadding],

    [button.trailingAnchor
        constraintEqualToAnchor:contentView.trailingAnchor
                       constant:-kPadding - kButtonRightPaddingAdjustment],
    [button.topAnchor constraintEqualToAnchor:_label.bottomAnchor
                                     constant:kLineSpace],
    [button.bottomAnchor
        constraintEqualToAnchor:contentView.bottomAnchor
                       constant:-kPadding - kButtonBottomPaddingAdjustment],
  ]];
}

- (UILabel*)label {
  return _label;
}

#pragma mark - Private

- (void)didTapRetry:(id)button {
  if (_onRetryCallback) {
    _onRetryCallback();
  }
}

@end
