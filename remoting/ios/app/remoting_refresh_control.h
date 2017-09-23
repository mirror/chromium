// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_
#define REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_

#import <UIKit/UIKit.h>

@interface RemotingRefreshControl : UIView

@property(nonatomic) void (^refreshCallback)(void);

@end

#endif  // REMOTING_IOS_APP_REMOTING_REFRESH_CONTROL_H_
