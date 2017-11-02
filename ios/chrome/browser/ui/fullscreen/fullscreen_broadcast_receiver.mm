// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_receiver.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FullscreenModelBroadcastReceiver ()
// The model being updated.
@property(nonatomic, readonly) FullscreenModel* model;
@end

@implementation FullscreenModelBroadcastReceiver
@synthesize scrollOffset = _scrollOffset;
@synthesize dragging = _dragging;
@synthesize scrolling = _scrolling;
@synthesize toolbarHeight = _toolbarHeight;
@synthesize model = _model;

- (instancetype)initWithModel:(FullscreenModel*)model {
  if (self = [super init]) {
    _model = model;
    DCHECK(_model);
  }
  return self;
}

#pragma mark - FullscreenBroadcastReceiving

- (void)setScrollOffset:(CGFloat)scrollOffset {
  if (_scrollOffset == scrollOffset)
    return;
  _scrollOffset = scrollOffset;
  self.model->SetYContentOffset(_scrollOffset);
}

- (void)setDragging:(BOOL)dragging {
  if (_dragging == dragging)
    return;
  _dragging = dragging;
  self.model->SetScrollViewIsDragging(_dragging);
}

- (void)setScrolling:(BOOL)scrolling {
  if (_scrolling == scrolling)
    return;
  _scrolling = scrolling;
  self.model->SetScrollViewIsScrolling(_scrolling);
}

- (void)setToolbarHeight:(CGFloat)toolbarHeight {
  if (_toolbarHeight == toolbarHeight)
    return;
  _toolbarHeight = toolbarHeight;
  self.model->SetToolbarHeight(_toolbarHeight);
}

@end
