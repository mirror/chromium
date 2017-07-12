// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_FIRST_LAUNCH_VIEW_PRESENTER_H_
#define REMOTING_IOS_APP_FIRST_LAUNCH_VIEW_PRESENTER_H_

#import <UIKit/UIKit.h>

#import "remoting/ios/app/first_launch_view_controller.h"

@interface FirstLaunchViewPresenter : NSObject

- (instancetype)initWithNavController:(UINavigationController*)navController
               viewControllerDelegate:
                   (id<FirstLaunchViewControllerDelegate>)delegate;

- (void)startTracking;

- (void)stopTracking;

// Forcibly present the first launch view.
- (void)presentView;

@end

#endif  // REMOTING_IOS_APP_FIRST_LAUNCH_VIEW_PRESENTER_H_
