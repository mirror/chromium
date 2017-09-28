// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_broadcast_forwarder.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FullscreenBroadcastForwarder ()<ChromeBroadcastObserver>
// The broadcaster passed on initialization.
@property(nonatomic, weak, readonly) ChromeBroadcaster* broadcaster;
// The model that receives forwarded scroll events.
@property(nonatomic, readonly) FullscreenModel* model;
@end

@implementation FullscreenBroadcastForwarder
@synthesize broadcaster = _broadcaster;
@synthesize model = _model;

- (instancetype)initWithBroadcaster:(ChromeBroadcaster*)broadcaster
                              model:(FullscreenModel*)model {
  if (self = [super init]) {
    _model = model;
    _broadcaster = broadcaster;
    DCHECK(_model);
    DCHECK(_broadcaster);
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
  self.model->SetYContentOffset(offset);
}

- (void)broadcastScrollViewIsScrolling:(BOOL)scrolling {
  self.model->SetScrollViewIsScrolling(scrolling);
}

- (void)broadcastScrollViewIsDragging:(BOOL)dragging {
  self.model->SetScrollViewIsDragging(dragging);
}

- (void)broadcastToolbarHeight:(CGFloat)height {
  self.model->SetToolbarHeight(height);
}

@end
