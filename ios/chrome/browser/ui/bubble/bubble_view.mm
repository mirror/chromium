// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

CGFloat kBubbleAlignmentOffset = 10.0f;

@interface BubbleView ()
// Label containing the text displayed on the bubble.
@property(nonatomic, weak) UILabel* label;
@end

@implementation BubbleView

@synthesize label = _label;

- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)direction
                   alignment:(BubbleAlignment)alignment {
  self = [super initWithFrame:CGRectZero];
  [self setupViews];
  return self;
}

- (void)setupViews {
  UIView* arrow = [self drawArrow];
  [self addSubview:arrow];

  UIView* background =
      [[UIView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 100.0f)];
  background.backgroundColor = [UIColor blueColor];
  [self addSubview:background];

  // add autolayout constraints
  [NSLayoutConstraint activateConstraints:@[
    [arrow.centerXAnchor constraintEqualToAnchor:self.trailingAnchor
                                        constant:kBubbleAlignmentOffset],
    [arrow.topAnchor constraintEqualToAnchor:self.topAnchor constant:8.0f],
    [arrow.widthAnchor constraintEqualToConstant:14.0f],
    [arrow.heightAnchor constraintEqualToConstant:10.0f],

    [background.topAnchor constraintEqualToAnchor:arrow.bottomAnchor],
    [background.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
    [background.widthAnchor constraintEqualToConstant:200.0f],
    [background.heightAnchor constraintEqualToConstant:100.0f],
  ]];
}

- (UIView*)drawArrow {
  CGFloat height = 10.0f;
  CGFloat width = 14.0f;
  CGRect rect = CGRectMake(0.0f, 0.0f, width, height);
  UIView* arrow = [[UIView alloc] initWithFrame:rect];
  UIBezierPath* path = UIBezierPath.bezierPath;
  [path moveToPoint:CGPointMake(width / 2.0f, 0.0f)];
  [path addLineToPoint:CGPointMake(0.0f, height)];
  [path addLineToPoint:CGPointMake(width, height)];
  [path closePath];

  CAShapeLayer* layer = [CAShapeLayer layer];
  layer.frame = rect;
  layer.path = path.CGPath;
  layer.fillColor = [UIColor blueColor].CGColor;
  [arrow.layer addSublayer:layer];

  return arrow;
}

#pragma mark - UIView overrides

// Override sizeThatFits to return the bubble's optimal size.
- (CGSize)sizeThatFits:(CGSize)size {
  NOTIMPLEMENTED();
  return size;
}

@end
