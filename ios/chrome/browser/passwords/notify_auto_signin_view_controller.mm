// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/notify_auto_signin_view_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

constexpr int kBackgroundColor = 0X4285F4;

}  // namespace

@interface NotifyUserAutoSigninViewController ()

@property(nonatomic) UIView* contentView;

@end

@implementation NotifyUserAutoSigninViewController

@synthesize contentView = _contentView;

@synthesize userID = _userID;

- (instancetype)initWithUserID:(NSString*)userID {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _userID = userID;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  // Catch user's tap.
  self.view.userInteractionEnabled = YES;
  self.view.backgroundColor = UIColorFromRGB(kBackgroundColor);
  [self.view setTranslatesAutoresizingMaskIntoConstraints:NO];

  // Blue background view for notification.
  _contentView = [[UIView alloc] init];
  [_contentView setTranslatesAutoresizingMaskIntoConstraints:NO];
  _contentView.backgroundColor = [UIColor clearColor];
  _contentView.userInteractionEnabled = NO;

  // View containing "Signing in as ..." text.
  UILabel* textView = [[UILabel alloc] init];
  [textView setTranslatesAutoresizingMaskIntoConstraints:NO];
  UIFont* font = [MDCTypography body1Font];
  textView.text =
      l10n_util::GetNSStringF(IDS_IOS_CREDENTIAL_MANAGER_AUTO_SIGNIN,
                              base::SysNSStringToUTF16(_userID));
  textView.font = font;
  textView.textColor = [UIColor whiteColor];

  // Add subiews
  [_contentView addSubview:textView];
  [self.view addSubview:_contentView];

  // Text view must leave 48pt on the left for user's avatar. Set the
  // constraints.
  NSDictionary* childrenViewsDictionary = @{
    @"textView" : textView,
  };
  NSArray* childrenConstraints = @[
    @"V:|[textView]|",
    @"H:|-48-[textView]-12-|",
  ];
  ApplyVisualConstraints(childrenConstraints, childrenViewsDictionary);
  [textView layoutIfNeeded];

  if (@available(iOS 11.0, *)) {
    // Show in safe area on iOS11, to respect iPhone X's shape.
    [NSLayoutConstraint activateConstraints:@[
      [_contentView.leadingAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.leadingAnchor],
      [_contentView.trailingAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.trailingAnchor],
      [_contentView.bottomAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor],
      [_contentView.topAnchor
          constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
    ]];
  } else {
    // Notification must be screen-wide and appear from the bottom. Set the
    // constraints.
    AddSameConstraints(_contentView, self.view);
  }
}

- (void)didMoveToParentViewController:(UIViewController*)parent {
  [super didMoveToParentViewController:parent];
  if (parent == nil) {
    return;
  }

  // Set constraints for blue background.
  [NSLayoutConstraint activateConstraints:@[
    [self.view.bottomAnchor
        constraintEqualToAnchor:self.view.superview.bottomAnchor],
    [self.view.leadingAnchor
        constraintEqualToAnchor:self.view.superview.leadingAnchor],
    [self.view.trailingAnchor
        constraintEqualToAnchor:self.view.superview.trailingAnchor],
    [_contentView.heightAnchor constraintEqualToConstant:48],
  ]];
}

@end
