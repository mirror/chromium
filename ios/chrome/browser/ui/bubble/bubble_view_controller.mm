// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_view_controller.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BubbleViewController ()
@property(nonatomic) BubbleView* bubbleView;
@property(nonatomic, readonly) NSString* text;
@property(nonatomic, readonly) BubbleArrowDirection arrowDirection;
@end

@implementation BubbleViewController

@synthesize bubbleView = _bubbleView;
@synthesize text = _text;
@synthesize arrowDirection = _arrowDirection;

- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)direction {
  self = [super initWithNibName:nil bundle:nil];
  _text = text;
  _arrowDirection = direction;
  return self;
}

- (void)loadView {
  self.bubbleView = [[BubbleView alloc] initWithText:self.text
                                      arrowDirection:self.arrowDirection];
  self.view = self.bubbleView;
}

- (void)anchorOnPoint:(CGPoint)anchorPoint {
  [self.bubbleView anchorOnPoint:anchorPoint
                  arrowDirection:self.arrowDirection];
}

- (void)animateContentIn {
  NOTIMPLEMENTED();
}

- (void)dismissAnimated:(BOOL)animated {
  NOTIMPLEMENTED();
}

@end
