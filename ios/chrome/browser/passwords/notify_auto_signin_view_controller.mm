// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/notify_auto_signin_view_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NotifyUserAutoSigninViewController {
  UIView* contentView;
  UILabel* textView;
}

constexpr NSString* kFontName = @"Roboto-Medium";
constexpr int kFontSize = 14;
constexpr int kBackgroundColor = 0X4285F4;

@synthesize userId = _userId;

- (instancetype)initWithUserId:(NSString*)userId {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _userId = userId;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  self.view.userInteractionEnabled = NO;
  self.view.backgroundColor = [UIColor clearColor];

  // Blue background view for notification.
  contentView = [[UIView alloc] init];
  [contentView setTranslatesAutoresizingMaskIntoConstraints:NO];
  contentView.backgroundColor = UIColorFromRGB(kBackgroundColor);
  contentView.userInteractionEnabled = NO;

  // View containing "Signing in as ..." text.
  textView = [[UILabel alloc] init];
  [textView setTranslatesAutoresizingMaskIntoConstraints:NO];
  UIFont* font = [UIFont fontWithName:kFontName size:kFontSize];
  textView.text =
      l10n_util::GetNSStringF(IDS_IOS_CREDENTIAL_MANAGER_AUTO_SIGNIN,
                              base::SysNSStringToUTF16(_userId));
  textView.font = font;
  textView.textColor = [UIColor whiteColor];

  // Add subiews
  [contentView addSubview:textView];
  [self.view addSubview:contentView];

  // Text view must leave 48pt on the left for user's avatar. Set the
  // constraints.
  NSDictionary* childrenViewsDictionary = @{
    @"textView" : textView,
  };
  NSArray* childrenConstraints = @[
    @"V:|[textView]|",
    @"H:|-48-[textView]-12-|",
  ];
  ApplyVisualConstraintsWithOptions(childrenConstraints,
                                    childrenViewsDictionary,
                                    LayoutOptionForRTLSupport());
  [textView layoutIfNeeded];
}

- (void)didMoveToParentViewController:(UIViewController*)parent {
  [super didMoveToParentViewController:parent];
  if (parent == nil) {
    return;
  }

  // Set notification bar's height to 48dp.
  [NSLayoutConstraint activateConstraints:@[
    [self.view.bottomAnchor
        constraintEqualToAnchor:self.view.superview.bottomAnchor],
    [contentView.heightAnchor constraintEqualToConstant:48],
  ]];

  if (@available(iOS 11.0, *)) {
    // Show in safe area on iOS11, to respect iPhone X's shape.
    [NSLayoutConstraint activateConstraints:@[
      [contentView.leadingAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor],
      [contentView.trailingAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor],
      [contentView.bottomAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
    ]];
  } else {
    // Notification must be screen-wide and appear from the bottom. Set the
    // constraints.
    [NSLayoutConstraint activateConstraints:@[
      [contentView.leadingAnchor
          constraintEqualToAnchor:self.view.leadingAnchor],
      [contentView.trailingAnchor
          constraintEqualToAnchor:self.view.trailingAnchor],
      [contentView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    ]];
  }
}

- (void)willMoveToParentViewController:(UIViewController*)parent {
  if (parent == nil) {
    [NSLayoutConstraint deactivateConstraints:self.view.constraints];
  }
  [super willMoveToParentViewController:parent];
}

@end
