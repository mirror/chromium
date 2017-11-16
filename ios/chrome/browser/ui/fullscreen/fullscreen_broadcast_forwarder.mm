// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_forwarder.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_receiver.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FullscreenBroadcastForwarder ()<ChromeBroadcastObserver>
// The broadcaster passed on initialization.
@property(nonatomic, weak, readonly) ChromeBroadcaster* broadcaster;
// The model that receives forwarded scroll events.
@property(nonatomic, readonly, weak) id<FullscreenBroadcastReceiving> receiver;
@end

@implementation FullscreenBroadcastForwarder
@synthesize broadcaster = _broadcaster;
@synthesize receiver = _receiver;

- (instancetype)initWithBroadcaster:(ChromeBroadcaster*)broadcaster
                           receiver:(id<FullscreenBroadcastReceiving>)receiver {
  if (self = [super init]) {
    _broadcaster = broadcaster;
    DCHECK(_broadcaster);
    _receiver = receiver;
    [_broadcaster addObserver:self
                  forSelector:@selector(broadcastContentScrollOffset:)];
    [_broadcaster addObserver:self
                  forSelector:@selector(broadcastScrollViewIsScrolling:)];
    [_broadcaster addObserver:self
                  forSelector:@selector(broadcastScrollViewIsDragging:)];
    [_broadcaster addObserver:self
                  forSelector:@selector(broadcastToolbarHeight:)];
  }
  return self;
}

#pragma mark - Public

- (void)disconnect {
  [self.broadcaster removeObserver:self
                       forSelector:@selector(broadcastContentScrollOffset:)];
  [self.broadcaster removeObserver:self
                       forSelector:@selector(broadcastScrollViewIsScrolling:)];
  [self.broadcaster removeObserver:self
                       forSelector:@selector(broadcastScrollViewIsDragging:)];
  [self.broadcaster removeObserver:self
                       forSelector:@selector(broadcastToolbarHeight:)];
}

#pragma mark - ChromeBroadcastObserver

- (void)broadcastContentScrollOffset:(CGFloat)offset {
  self.receiver.scrollOffset = offset;
}

- (void)broadcastScrollViewIsScrolling:(BOOL)scrolling {
  self.receiver.scrolling = scrolling;
}

- (void)broadcastScrollViewIsDragging:(BOOL)dragging {
  self.receiver.dragging = dragging;
}

- (void)broadcastToolbarHeight:(CGFloat)height {
  self.receiver.toolbarHeight = height;
}

@end
