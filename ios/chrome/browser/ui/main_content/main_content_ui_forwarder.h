// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_UI_FORWARDER_H_
#define IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_UI_FORWARDER_H_

#import <UIKit/UIKit.h>

@class ChromeBroadcaster;

// Helper object that uses a ChromeBroadcaster to broadcast UI state for the
// main scrollable content area.  The UI that displays the main content area
// should use UIScrollViewDelegate callbacks to pass scrolling state to this
// object.
@interface MainContentUIForwarder : NSObject

// Whether the scroll forwarder is currently broadcasting.
@property(nonatomic, readonly, getter=isBroadcasting) BOOL broadcasting;

// Starts broadcasting scrolling information using |broadcaster|.
- (void)startBroadcasting:(ChromeBroadcaster*)broadcaster;
- (void)stopBroadcasting:(ChromeBroadcaster*)broadcaster;

// Called to broadcast scroll offset changes due to scrolling.
- (void)scrollViewDidScrollToOffset:(CGPoint)offset;
// Called when a drag event with |panGesture| begins.
- (void)scrollViewWillBeginDragging:(UIPanGestureRecognizer*)panGesture;
// Called when a drag event with |panGesture| ends.  |velocity| is the regidual
// velocity from the scroll event.
- (void)scrollViewDidEndDragging:(UIPanGestureRecognizer*)panGesture
                residualVelocity:(CGPoint)velocity;
// Called when the scroll view stops decelerating.
- (void)scrollViewDidEndDecelerating;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_UI_FORWARDER_H_
