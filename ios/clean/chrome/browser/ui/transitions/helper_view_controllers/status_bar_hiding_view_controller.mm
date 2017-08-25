// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/transitions/helper_view_controllers/status_bar_hiding_view_controller.h"

@implementation StatusBarHidingViewController

- (instancetype)initWithChildViewController:(UIViewController*)child
                             statusBarColor:(UIColor*)color {
  self = [super init];
  if (self) {
    UIView* statusMaskView = [[UIView alloc] init];
    [self addChildViewController:child];
    statusMaskView.backgroundColor = color;
    statusMaskView.translatesAutoresizingMaskIntoConstraints = NO;
    child.view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:child.view];
    [self.view addSubview:statusMaskView];
    CGFloat statusBarHeight =
        [UIApplication sharedApplication].statusBarFrame.size.height;
    [NSLayoutConstraint activateConstraints:@[
      [statusMaskView.leftAnchor constraintEqualToAnchor:self.view.leftAnchor],
      [statusMaskView.rightAnchor
          constraintEqualToAnchor:self.view.rightAnchor],
      [statusMaskView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
      [statusMaskView.heightAnchor constraintEqualToConstant:statusBarHeight],
      [child.view.leftAnchor constraintEqualToAnchor:self.view.leftAnchor],
      [child.view.rightAnchor constraintEqualToAnchor:self.view.rightAnchor],
      [child.view.topAnchor
          constraintEqualToAnchor:statusMaskView.bottomAnchor],
      [child.view.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    ]];
  }
  return self;
}

@end
