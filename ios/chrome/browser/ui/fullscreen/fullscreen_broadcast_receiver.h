// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_BROADCAST_RECEIVER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_BROADCAST_RECEIVER_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

class FullscreenModel;

// Protocol for objects interested in receiving broadcasted fullscreen values.
@protocol FullscreenBroadcastReceiving<NSObject>
@property(nonatomic) CGFloat scrollOffset;
@property(nonatomic, getter=isDragging) BOOL dragging;
@property(nonatomic, getter=isScrolling) BOOL scrolling;
@property(nonatomic) CGFloat toolbarHeight;
@end

// FullscreenBroadcastReceiver that forwards UI state to a FullscreenModel.
@interface FullscreenModelBroadcastReceiver
    : NSObject<FullscreenBroadcastReceiving>

// Designated initializer.
- (nullable instancetype)initWithModel:(nonnull FullscreenModel*)model
    NS_DESIGNATED_INITIALIZER;
- (nullable instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_BROADCAST_RECEIVER_H_
