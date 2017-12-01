// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/clipping_mask_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - GradientView

// The gradient view is a view that displays a simple horizontal gradient, from
// black to transparent.
@interface GradientView : UIView
// Sets the gradient's direction. You have to call this at least once to
// configure the gradient.
@property(nonatomic, assign) BOOL leftToRight;
@end

@implementation GradientView
@synthesize leftToRight = _leftToRight;

+ (Class)layerClass {
  return [CAGradientLayer class];
}

- (void)setLeftToRight:(BOOL)leftToRight {
  _leftToRight = leftToRight;

  CAGradientLayer* layer = (CAGradientLayer*)self.layer;
  layer.colors = @[
    (id)[[UIColor clearColor] CGColor], (id)[[UIColor blackColor] CGColor]
  ];

  if (leftToRight) {
    layer.transform = CATransform3DMakeRotation(M_PI_2, 0, 0, 1);
  } else {
    layer.transform = CATransform3DMakeRotation(-M_PI_2, 0, 0, 1);
  }

  [self setNeedsDisplay];
}

@end

#pragma mark - ClippingMaskView

@interface ClippingMaskView ()
// The left gradient view.
@property(nonatomic, strong) GradientView* leftGradView;
// The right gradient view.
@property(nonatomic, strong) GradientView* rightGradView;
// The middle view. Used to paint everything not covered by the gradients with
// opaque color.
@property(nonatomic, strong) UIView* middleView;
@end

@implementation ClippingMaskView
@synthesize fadeLeft = _fadeLeft;
@synthesize fadeRight = _fadeRight;
@synthesize middleView = _middleView;
@synthesize leftGradView = _leftGradView;
@synthesize rightGradView = _rightGradView;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _leftGradView = [[GradientView alloc] init];
    [_leftGradView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _leftGradView.leftToRight = NO;

    _rightGradView = [[GradientView alloc] init];
    [_rightGradView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _rightGradView.leftToRight = YES;

    _middleView = [[UIView alloc] init];
    [_middleView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _middleView.backgroundColor = [UIColor blackColor];

    [self addSubview:_leftGradView];
    [self addSubview:_middleView];
    [self addSubview:_rightGradView];
  }
  return self;
}

- (void)setFadeLeft:(BOOL)fadeLeft {
  _fadeLeft = fadeLeft;
  self.leftGradView.hidden = !fadeLeft;
}

- (void)setFadeRight:(BOOL)fadeRight {
  _fadeRight = fadeRight;
  self.rightGradView.hidden = !fadeRight;
}

// Since this view is intended to be used as a maskView of another UIView, and
// maskViews are not in the view hierarchy, autolayout support is really poor.
// Luckily, the desired layout is easy:
// |[leftGradView][middleView][rightGradView]|,
// where the grad views are sometimes hidden (and then the middle view is
// extended to that side). The gradients' widths are equal to the height of this
// view.
- (void)layoutSubviews {
  [super layoutSubviews];
  CGFloat height = self.bounds.size.height;
  CGFloat gradientWidth = height;
  CGFloat xOffset = 0;
  if ([self fadeLeft]) {
    self.leftGradView.frame = CGRectMake(0, 0, gradientWidth, height);
    xOffset += gradientWidth;
  }

  CGFloat midWidth = self.bounds.size.width - xOffset;
  if ([self fadeRight]) {
    midWidth -= gradientWidth;
  }

  self.middleView.frame = CGRectMake(xOffset, 0, midWidth, height);
  xOffset += midWidth;

  if ([self fadeRight]) {
    self.rightGradView.frame = CGRectMake(xOffset, 0, gradientWidth, height);
  }
}

@end
