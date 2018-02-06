// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_APP_UPDATE_MONITOR_H_
#define REMOTING_IOS_APP_UPDATE_MONITOR_H_

#import <UIKit/UIKit.h>

// This is the base class for monitoring app updates. The base implementation
// does nothing.
@interface UpdateMonitor : NSObject

// Starts monitoring app updates.
- (void)start;

// Instance can only be set once.
@property(nonatomic, class) UpdateMonitor* instance;

@end

#endif  // REMOTING_IOS_APP_UPDATE_MONITOR_H_
