// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/notify_auto_signin_view_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NotifyUserAutoSigninViewController

@synthesize userId = _userId;

- (instancetype)initWithUserId:(NSString*)userId {
  self = [super initWithNibName:nil bundle:nil];
  self.userId = [userId copy];
  self.view.userInteractionEnabled = NO;
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  // Constraints are used to determine the geometry. This is only for
  // initializer.
  CGRect zeroRect = CGRectMake(0, 0, 0, 0);
  // Blue background view for notification.
  UIView* bgView = [[UIView alloc] initWithFrame:zeroRect];
  [bgView setTranslatesAutoresizingMaskIntoConstraints:NO];
  self.view.backgroundColor = [UIColor clearColor];
  // Sets background color to #4285F4
  bgView.backgroundColor =
      [UIColor colorWithRed:0.26 green:0.52 blue:0.96 alpha:1.0];
  bgView.userInteractionEnabled = NO;
  // View containing "Signing in as ..." text.
  UILabel* textView = [[UILabel alloc] initWithFrame:zeroRect];
  [textView setTranslatesAutoresizingMaskIntoConstraints:NO];
  UIFont* font = [UIFont fontWithName:@"Roboto-Medium" size:14];
  textView.text =
      l10n_util::GetNSStringF(IDS_IOS_CREDENTIAL_MANAGER_AUTO_SIGNIN,
                              base::SysNSStringToUTF16(_userId));
  textView.font = font;
  textView.textColor = [UIColor whiteColor];
  [bgView addSubview:textView];

  // Text view must leave 48dp on the left for user's avatar. Set the
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

  // Update the views.
  [textView layoutIfNeeded];
  [bgView layoutIfNeeded];

  // Background view must be screen-wide and 48dp high. Set the constraints.
  [self.view addSubview:bgView];
  NSDictionary* viewsDictionary = @{
    @"bgView" : bgView,
  };
  NSArray* constraints = @[
    @"V:[bgView(==48)]-0-|",
    @"H:|-0-[bgView]-0-|",
  ];
  ApplyVisualConstraintsWithOptions(constraints, viewsDictionary,
                                    LayoutOptionForRTLSupport());
}

@end
