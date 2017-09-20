// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/notify_auto_signin_view_controller.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#include "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NotifyUserAutoSigninViewController

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

  // Blue background view for notification.
  UIView* backgroundView = [[UIView alloc] init];
  [backgroundView setTranslatesAutoresizingMaskIntoConstraints:NO];
  self.view.backgroundColor = [UIColor clearColor];
  backgroundView.backgroundColor = UIColorFromRGB(kBackgroundColor);
  backgroundView.userInteractionEnabled = NO;
  // View containing "Signing in as ..." text.
  UILabel* textView = [[UILabel alloc] init];
  [textView setTranslatesAutoresizingMaskIntoConstraints:NO];
  UIFont* font = [UIFont fontWithName:kFontName size:kFontSize];
  textView.text =
      l10n_util::GetNSStringF(IDS_IOS_CREDENTIAL_MANAGER_AUTO_SIGNIN,
                              base::SysNSStringToUTF16(_userId));
  textView.font = font;
  textView.textColor = [UIColor whiteColor];
  [backgroundView addSubview:textView];

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

  // Update the views.
  [textView layoutIfNeeded];

  // Background view must be screen-wide and 48pt high. Set the constraints.
  [self.view addSubview:backgroundView];
  NSDictionary* viewsDictionary = @{
    @"backgroundView" : backgroundView,
  };
  NSArray* constraints = @[
    @"V:[backgroundView(==48)]-0-|",
    @"H:|-0-[backgroundView]-0-|",
  ];
  ApplyVisualConstraintsWithOptions(constraints, viewsDictionary,
                                    LayoutOptionForRTLSupport());
}

@end
