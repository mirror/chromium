// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/host_fetching_view_controller.h"

#import "remoting/ios/app/remoting_refresh_control.h"
#import "remoting/ios/app/remoting_theme.h"

@implementation HostFetchingViewController

- (void)viewDidLoad {
  MDCActivityIndicator* activityIndicator =
      [RemotingRefreshControl createRefreshIndicator];
  activityIndicator.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:activityIndicator];
  [NSLayoutConstraint activateConstraints:@[
    [activityIndicator.centerXAnchor
        constraintEqualToAnchor:self.view.centerXAnchor],
    [activityIndicator.centerYAnchor
        constraintEqualToAnchor:self.view.centerYAnchor],
  ]];
  [activityIndicator startAnimating];
}

@end
