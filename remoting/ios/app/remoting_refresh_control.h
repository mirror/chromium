// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_
#define REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_

#import <UIKit/UIKit.h>

#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MDCActivityIndicator.h"

@interface RemotingRefreshControl : UIView

- (void)endRefreshing;

+ (MDCActivityIndicator*)createRefreshIndicator;

@property(nonatomic) void (^refreshCallback)();
@property(nonatomic, weak) UIScrollView* trackingScrollView;
@property(nonatomic, readonly) BOOL isRefreshing;

@end

#endif  // REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_
