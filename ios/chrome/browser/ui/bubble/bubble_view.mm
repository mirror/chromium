// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const CGFloat kBubbleAlignmentOffset = 40.0f;

namespace {
UIColor* const kBubbleColor = [[MDCPalette cr_bluePalette] tint500];
const CGFloat kBubbleCornerRadius = 18.0f;
const CGFloat kMinBubbleWidth = 80.0f;
const CGFloat kMaxLabelWidth = 359.0f;
const CGFloat kFontSize = 16.0f;
const CGFloat kMargin = 8.0f;

const CGFloat kArrowWidth = 14.0f;
const CGFloat kArrowHeight = 10.0f;
// The factor by which the arrow's size is scaled. Drawing the arrow larger than
// its desired size prevents gaps between the arrow and the background view.
const CGFloat kArrowScaleFactor = 1.5f;

const CGSize kShadowOffset = CGSizeMake(0.0f, 2.0f);
const CGFloat kShadowRadius = 4.0f;
const CGFloat kShadowOpacity = 0.1f;
}  // namespace

@interface BubbleView ()
// Label containing the text displayed on the bubble.
@property(nonatomic, weak) UILabel* label;
// Pill-shaped view in the background of the bubble.
@property(nonatomic, weak, readonly) UIView* background;
// Triangular arrow that points to the target UI element.
@property(nonatomic, weak, readonly) UIView* arrow;

@property(nonatomic, readonly) BubbleArrowDirection direction;
@property(nonatomic, readonly) BubbleAlignment alignment;
@end

@implementation BubbleView
@synthesize label = _label;
@synthesize background = _background;
@synthesize arrow = _arrow;
@synthesize direction = _direction;
@synthesize alignment = _alignment;

- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)direction
                   alignment:(BubbleAlignment)alignment {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    [self setAccessibilityLabel:text];
    _direction = direction;
    _alignment = alignment;

    UIView* arrow = [self arrowView];
    _arrow = arrow;
    [self addSubview:arrow];

    UIView* background = [self backgroundView];
    _background = background;
    [self addSubview:background];

    UILabel* label = [self labelWithText:text];
    self.label = label;
    [self addSubview:label];

    [self activateConstraints];
    [self setShadow];
  }
  return self;
}

#pragma mark - Private

// Return a triangular view to be used as the arrow of the bubble.
- (UIView*)arrowView {
  // Draw the arrow slightly larger than the desired size in order to prevent
  // gaps between the background view and the arrow.
  CGFloat width = kArrowWidth * kArrowScaleFactor;
  CGFloat height = kArrowHeight * kArrowScaleFactor;
  UIView* arrow =
      [[UIView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, width, height)];
  UIBezierPath* path = UIBezierPath.bezierPath;
  if (self.direction == BubbleArrowDirectionUp) {
    [path moveToPoint:CGPointMake(width / 2.0f, 0.0f)];
    [path addLineToPoint:CGPointMake(0.0f, height)];
    [path addLineToPoint:CGPointMake(width, height)];
  } else {
    DCHECK(self.direction == BubbleArrowDirectionDown);
    [path moveToPoint:CGPointMake(width / 2.0f, height)];
    [path addLineToPoint:CGPointMake(0.0f, 0.0f)];
    [path addLineToPoint:CGPointMake(width, 0.0f)];
  }
  [path closePath];

  CAShapeLayer* layer = [CAShapeLayer layer];
  [layer setPath:path.CGPath];
  [layer setFillColor:kBubbleColor.CGColor];
  [arrow.layer addSublayer:layer];
  [arrow setTranslatesAutoresizingMaskIntoConstraints:NO];
  return arrow;
}

// Return a blue, pill-shaped view to be used as the background of the bubble.
- (UIView*)backgroundView {
  UIView* background = [[UIView alloc] initWithFrame:CGRectZero];
  [background setBackgroundColor:kBubbleColor];
  [background.layer setCornerRadius:kBubbleCornerRadius];
  [background setTranslatesAutoresizingMaskIntoConstraints:NO];
  return background;
}

// Return a label that displays white text.
- (UILabel*)labelWithText:(NSString*)text {
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
  [label setText:text];
  [label setFont:[UIFont systemFontOfSize:kFontSize]];
  [label setTextColor:[UIColor whiteColor]];
  [label setTextAlignment:NSTextAlignmentCenter];
  [label setNumberOfLines:0];
  [label setLineBreakMode:NSLineBreakByWordWrapping];
  [label setTranslatesAutoresizingMaskIntoConstraints:NO];
  return label;
}

// Add a drop shadow to the bubble.
- (void)setShadow {
  [self.layer setShadowOffset:kShadowOffset];
  [self.layer setShadowRadius:kShadowRadius];
  [self.layer setShadowColor:[UIColor blackColor].CGColor];
  [self.layer setShadowOpacity:kShadowOpacity];
}

// Activate Autolayout constraints to properly position the bubble's subviews.
- (void)activateConstraints {
  // Add constraints that do not depend on the bubble's direction or alignment.
  NSMutableArray<NSLayoutConstraint*>* constraints =
      [NSMutableArray arrayWithArray:@[
        // Ensure that the bubble view is wider than the background view, and
        // add padding to the sides of the background.
        [self.widthAnchor
            constraintGreaterThanOrEqualToAnchor:self.background.widthAnchor
                                        constant:kMargin * 2],
        [self.background.widthAnchor
            constraintGreaterThanOrEqualToAnchor:self.label.widthAnchor
                                        constant:kBubbleCornerRadius],
        // Enforce the minimum width of the background view.
        [self.background.widthAnchor
            constraintGreaterThanOrEqualToConstant:kMinBubbleWidth -
                                                   kMargin * 2],
        [self.background.heightAnchor
            constraintGreaterThanOrEqualToAnchor:self.label.heightAnchor
                                        constant:kMargin * 2],
        [self.label.centerXAnchor
            constraintEqualToAnchor:self.background.centerXAnchor],
        [self.label.centerYAnchor
            constraintEqualToAnchor:self.background.centerYAnchor],
        [self.arrow.widthAnchor
            constraintEqualToConstant:kArrowWidth * kArrowScaleFactor],
        [self.arrow.heightAnchor
            constraintEqualToConstant:kArrowHeight * kArrowScaleFactor]
      ]];

  // Add constraints that depend on the bubble's direction.
  if (self.direction == BubbleArrowDirectionUp) {
    [constraints addObjectsFromArray:@[
      [self.background.topAnchor constraintEqualToAnchor:self.arrow.topAnchor
                                                constant:kArrowHeight],
      // Ensure that the top of the arrow is aligned with the top of the bubble
      // view and add padding above the arrow.
      [self.arrow.topAnchor constraintEqualToAnchor:self.topAnchor
                                           constant:kMargin]
    ]];
  } else {
    DCHECK(self.direction == BubbleArrowDirectionDown);
    [constraints addObjectsFromArray:@[
      [self.arrow.bottomAnchor
          constraintEqualToAnchor:self.background.bottomAnchor
                         constant:kArrowHeight],
      // Ensure that the bottom of the arrow is aligned with the bottom of the
      // bubble view and add padding below the arrow.
      [self.bottomAnchor constraintEqualToAnchor:self.arrow.bottomAnchor
                                        constant:kMargin]
    ]];
  }

  // Add constraints that depend on the bubble's alignment.
  switch (self.alignment) {
    case BubbleAlignmentLeading:
      [constraints addObjectsFromArray:@[
        [self.arrow.centerXAnchor
            constraintEqualToAnchor:self.leadingAnchor
                           constant:kBubbleAlignmentOffset],
        [self.background.leadingAnchor
            constraintEqualToAnchor:self.leadingAnchor
                           constant:kMargin]
      ]];
      break;
    case BubbleAlignmentCenter:
      [constraints addObjectsFromArray:@[
        [self.centerXAnchor constraintEqualToAnchor:self.arrow.centerXAnchor],
        [self.background.centerXAnchor
            constraintEqualToAnchor:self.centerXAnchor]
      ]];
      break;
    case BubbleAlignmentTrailing:
      [constraints addObjectsFromArray:@[
        [self.trailingAnchor constraintEqualToAnchor:self.arrow.centerXAnchor
                                            constant:kBubbleAlignmentOffset],
        [self.trailingAnchor
            constraintEqualToAnchor:self.background.trailingAnchor
                           constant:kMargin],
      ]];
      break;
    default:
      NOTREACHED() << "Invalid bubble alignment " << self.alignment;
      break;
  }

  [NSLayoutConstraint activateConstraints:constraints];
}

#pragma mark - UIView overrides

// Override |sizeThatFits| to return the bubble's optimal size. Calculate
// optimal size by finding the label's optimal size, and adding inset distances
// to the label's dimensions.
- (CGSize)sizeThatFits:(CGSize)size {
  // The combined horizontal inset distance of the label with respect to the
  // bubble.
  CGFloat labelHorizontalInset = (kMargin + kBubbleCornerRadius) * 2;
  // The combined vertical inset distance of the label with respect to the
  // bubble.
  CGFloat labelVerticalInset = kMargin * 3 + kArrowHeight;
  // Calculate the maximum width the label is allowed to use, and ensure that
  // the label doesn't exceed the maximum line width.
  CGFloat labelMaxWidth =
      MIN(size.width - labelHorizontalInset, kMaxLabelWidth);
  CGSize labelMaxSize =
      CGSizeMake(labelMaxWidth, size.height - labelVerticalInset);
  CGSize labelSize = [self.label sizeThatFits:labelMaxSize];
  // Ensure that the bubble is at least as wide as the minimum bubble width.
  CGFloat optimalWidth =
      MAX(labelSize.width + labelHorizontalInset, kMinBubbleWidth);
  CGSize optimalSize =
      CGSizeMake(optimalWidth, labelSize.height + labelVerticalInset);
  return optimalSize;
}

@end
