// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main_content/main_content_ui_forwarder.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MainContentUIForwarder ()
// The broadcaster currently being used.
@property(nonatomic, strong) ChromeBroadcaster* broadcaster;
// Broadcasted properties.
@property(nonatomic, assign) CGFloat yContentOffset;
@property(nonatomic, assign, getter=isScrolling) BOOL scrolling;
@property(nonatomic, assign, getter=isDragging) BOOL dragging;
// Whether the scroll view is decelerating.
@property(nonatomic, assign, getter=isDecelerating) BOOL decelerating;
// The gesture driving the current drag event.
@property(nonatomic, weak) UIPanGestureRecognizer* panGesture;

// Updates |scrolling| based |dragging| and |decelerating|.
- (void)updateIsScrolling;

@end

@implementation MainContentUIForwarder
@synthesize broadcaster = _broadcaster;
@synthesize yContentOffset = _yContentOffset;
@synthesize scrolling = _scrolling;
@synthesize dragging = _dragging;
@synthesize decelerating = _decelerating;
@synthesize panGesture = _panGesture;

- (BOOL)isBroadcasting {
  return self.broadcaster != nil;
}

- (void)setBroadcaster:(ChromeBroadcaster*)broadcaster {
  if (_broadcaster == broadcaster)
    return;
  [_broadcaster
      stopBroadcastingForSelector:@selector(broadcastContentScrollOffset:)];
  [_broadcaster
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsScrolling:)];
  [_broadcaster
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsDragging:)];
  _broadcaster = broadcaster;
  [_broadcaster broadcastValue:@"yContentOffset"
                      ofObject:self
                      selector:@selector(broadcastContentScrollOffset:)];
  [_broadcaster broadcastValue:@"scrolling"
                      ofObject:self
                      selector:@selector(broadcastScrollViewIsScrolling:)];
  [_broadcaster broadcastValue:@"dragging"
                      ofObject:self
                      selector:@selector(broadcastScrollViewIsDragging:)];
}

- (void)setDragging:(BOOL)dragging {
  if (_dragging == dragging)
    return;
  _dragging = dragging;
  [self updateIsScrolling];
}

- (void)setDecelerating:(BOOL)decelerating {
  if (_decelerating == decelerating)
    return;
  // If the scroll view is starting to decelerate after a drag, it is expected
  // that this property is set before |dragging| is reset to NO.  This ensures
  // that the broadcasted |scrolling| property does not quickly flip to NO when
  // drag events finish.
  DCHECK(!decelerating || self.dragging);
  _decelerating = decelerating;
  [self updateIsScrolling];
}

#pragma mark - Public

- (void)startBroadcasting:(ChromeBroadcaster*)broadcaster {
  DCHECK(!self.broadcasting);
  self.broadcaster = broadcaster;
}

- (void)stopBroadcasting:(ChromeBroadcaster*)broadcaster {
  DCHECK(self.broadcasting);
  DCHECK_EQ(self.broadcaster, broadcaster);
  self.broadcaster = nil;
}

- (void)scrollViewDidScrollToOffset:(CGPoint)offset {
  self.yContentOffset = offset.y;
}

- (void)scrollViewWillBeginDragging:(UIPanGestureRecognizer*)panGesture {
  self.dragging = YES;
  self.panGesture = panGesture;
}

- (void)scrollViewDidEndDragging:(UIPanGestureRecognizer*)panGesture
                residualVelocity:(CGPoint)velocity {
  DCHECK_EQ(panGesture, self.panGesture);
  if (!AreCGFloatsEqual(velocity.y, 0.0))
    self.decelerating = YES;
  self.dragging = NO;
  self.panGesture = nil;
}

- (void)scrollViewDidEndDecelerating {
  self.decelerating = NO;
}

#pragma mark - Private

- (void)updateIsScrolling {
  self.scrolling = self.dragging || self.decelerating;
}

@end
