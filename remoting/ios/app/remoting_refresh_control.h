// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_
#define REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_

#import <UIKit/UIKit.h>

#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MDCActivityIndicator.h"

// A control that implements pull-to-refresh on a scroll view.
@interface RemotingRefreshControl : UIView

// Tells the control that a refresh operation has ended.
- (void)endRefreshing;

// The callback to be run when the user triggers pull-to-refresh.
@property(nonatomic) void (^refreshCallback)();

// The scroll view to be tracked. The refresh control will be automatically
// added to the scroll view when this is set.
@property(nonatomic, weak) UIScrollView* trackingScrollView;

// A Boolean value indicating whether a refresh operation has been triggered and
// is in progress.
@property(nonatomic, readonly) BOOL isRefreshing;

@end

#endif  // REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_
