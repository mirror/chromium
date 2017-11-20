// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROADCASTER_CHROME_BROADCAST_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_BROADCASTER_CHROME_BROADCAST_OBSERVER_H_

#import <UIKit/UIKit.h>

// Group of properties commonly observed together for fullscreen events
@protocol FullscreenBroadcastObserver<NSObject>
@optional

@property(nonatomic) CGFloat contentScrollOffset;
@property(nonatomic) BOOL contentIsScrolling;
@property(nonatomic) BOOL contentIsDragging;

@end

@protocol ChromeBroadcastObserver<NSObject, FullscreenBroadcastObserver>
@optional

#pragma mark - Tab strip UI

@property(nonatomic) BOOL tabStripIsVisible;

//// Observer method for object that care about the current visibility of the
/// tab / strip.
//- (void)broadcastTabStripVisible:(BOOL)visible;
//
//#pragma mark - Scrolling events
//
//// Observer method for objects that care about the current vertical (y-axis)
//// scroll offset of the tab content area.
//- (void)broadcastContentScrollOffset:(CGFloat)offset;
//
//// Observer method for objects that care about whether the main content area
/// is / scrolling.
//- (void)broadcastScrollViewIsScrolling:(BOOL)scrolling;
//
//// Observer method for objects that care abotu whether the main content area
/// is / being dragged.  Note that if a drag ends with residual velocity, it's /
/// possible for |dragging| to be NO while |scrolling| is still YES.
//- (void)broadcastScrollViewIsDragging:(BOOL)dragging;

@end

#endif  // IOS_CHROME_BROWSER_UI_BROADCASTER_CHROME_BROADCAST_OBSERVER_H_
