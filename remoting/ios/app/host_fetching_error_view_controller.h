// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_HOST_FETCHING_ERROR_VIEW_CONTROLLER_H_
#define REMOTING_IOS_APP_HOST_FETCHING_ERROR_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

// This VC shows a view to allow the user to retry host list fetching by tapping
// a button. This is used when the host list fails to fetch when it's not
// triggered by a refresh control.
@interface HostFetchingErrorViewController : UIViewController

@property(nonatomic) void (^onRetryCallback)();
@property(nonatomic, readonly) UILabel* label;

@end

#endif  // REMOTING_IOS_APP_HOST_FETCHING_ERROR_VIEW_CONTROLLER_H_
