// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"

#include "base/logging.h"
#include "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/common/material_timing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kAnimationDuration = ios::material::kDuration3;
// The vertical offset distance used in the sink-down animation.
const CGFloat kVerticalOffset = 8.0f;
}  // namespace

@interface BubbleViewController ()
@property(nonatomic, readonly) NSString* text;
@property(nonatomic, readonly) BubbleArrowDirection arrowDirection;
@property(nonatomic, readonly) BubbleAlignment alignment;
@end

@implementation BubbleViewController
@synthesize text = _text;
@synthesize arrowDirection = _arrowDirection;
@synthesize alignment = _alignment;

- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)direction
                   alignment:(BubbleAlignment)alignment {
  self = [super initWithNibName:nil bundle:nil];
  _text = text;
  _arrowDirection = direction;
  _alignment = alignment;
  return self;
}

- (void)loadView {
  self.view = [[BubbleView alloc] initWithText:self.text
                                arrowDirection:self.arrowDirection
                                     alignment:self.alignment];
  // Begin hidden.
  self.view.alpha = 0.0;
}

// Animate the bubble view in with a fade-in and sink-down animation.
- (void)animateContentIn {
  // Set the frame's origin to be slightly higher on the screen, so that the
  // view will be properly positioned once it sinks down.
  __block CGRect frame = self.view.frame;
  frame.origin.y = frame.origin.y - kVerticalOffset;
  [self.view setFrame:frame];

  [UIView animateWithDuration:kAnimationDuration
                        delay:0.0f
                      options:UIViewAnimationOptionCurveEaseOut
                   animations:^{
                     // Set the y-coordinate of the frame's origin to its final
                     // value.
                     frame.origin.y = frame.origin.y + kVerticalOffset;
                     [self.view setFrame:frame];
                     self.view.alpha = 1.0;
                   }
                   completion:nil];
}

- (void)dismissAnimated:(BOOL)animated {
  if (animated) {
    [UIView animateWithDuration:kAnimationDuration
                     animations:^{
                       [self.view setAlpha:0.0f];
                     }];
  } else {
    [self.view setAlpha:0.0f];
  }
}

@end
