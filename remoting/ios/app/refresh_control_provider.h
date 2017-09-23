// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_REFRESH_CONTROL_PROVIDER_H_
#define REMOTING_IOS_APP_REFRESH_CONTROL_PROVIDER_H_

#import <UIKit/UIKit.h>

typedef void (^RemotingRefreshAction)();

@protocol RemotingRefreshControl<NSObject>

- (void)endRefreshing;

@property(nonatomic, readonly, getter=isRefreshing) BOOL refreshing;

@end

@interface RefreshControlProvider : NSObject

- (id<RemotingRefreshControl>)createForScrollView:(UIScrollView*)scrollView
                                      actionBlock:(RemotingRefreshAction)action;

@property(nonatomic, class) RefreshControlProvider* instance;

@end

#endif  // REMOTING_IOS_APP_REFRESH_CONTROL_PROVIDER_H_
