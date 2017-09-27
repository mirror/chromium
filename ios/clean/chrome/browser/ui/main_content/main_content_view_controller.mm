// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/main_content/main_content_view_controller.h"

#include "base/logging.h"
#import "ios/clean/chrome/browser/ui/main_content/main_content_view_controller+subclassing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MainContentViewController () {
  // Backing variables for properties of same name (defined in
  // MainContentSubclassing category).
  CGPoint _contentOffset;
  BOOL _decelerating;
}
// Redefine as readwrite.
@property(nonatomic, readwrite) CGFloat yContentOffset;
@property(nonatomic, readwrite, getter=isScrolling) BOOL scrolling;
@property(nonatomic, readwrite, getter=isDragging) BOOL dragging;
// Updates |scrolling| based on the current values of |dragging| and
// |decelerating|.
- (void)updateScrolling;
@end

@implementation MainContentViewController
@synthesize yContentOffset = _yContentOffset;
@synthesize scrolling = _scrolling;
@synthesize dragging = _dragging;

- (void)setDragging:(BOOL)dragging {
  if (_dragging == dragging)
    return;
  _dragging = dragging;
  [self updateScrolling];
}

#pragma mark - Private

- (void)updateScrolling {
  self.scrolling = self.dragging || self.decelerating;
}

@end

@implementation MainContentViewController (MainContentSubclassing)

- (CGPoint)contentOffset {
  return _contentOffset;
}

- (void)setContentOffset:(CGPoint)contentOffset {
  _contentOffset = contentOffset;
  self.yContentOffset = _contentOffset.y;
}

- (BOOL)isDecelerating {
  return _decelerating;
}

- (void)setDecelerating:(BOOL)decelerating {
  // For scroll events where a drag finishes resulting in some residual
  // velocity, |decelerating| should be set to YES before |dragging| is set to
  // NO.  The scroll event is completely finished once both properties are set
  // to NO.
  DCHECK(!decelerating || self.dragging);
  _decelerating = decelerating;
  [self updateScrolling];
}

@end
