// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/md_download_item_progress_indicator.h"

#import <QuartzCore/QuartzCore.h>

#import "base/mac/scoped_block.h"
#import "base/mac/scoped_cftyperef.h"
#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/md_util.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/color_palette.h"

namespace {

// An indeterminate progress indicator is a moving 50° arc…
constexpr CGFloat kIndeterminateArcSize = 50 / 360.0;
// …which completes a rotation around the circle every 4.5 seconds…
constexpr CGFloat kIndeterminateAnimationDuration = 4.5;
// …stored under this key.
NSString* const kIndeterminateAnimationKey = @"indeterminate";

// Width of the progress stroke itself.
constexpr CGFloat kStrokeWidth = 2;

base::ScopedCFTypeRef<CGPathRef> CirclePathWithBounds(CGRect bounds) {
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddArc(path, nullptr, NSMidX(bounds), NSMidY(bounds),
               NSWidth(bounds) / 2, 2 * M_PI + M_PI_2, M_PI_2, true);
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

// The line under the arrow goes through three paths, in sequence:

// 1. A line under the arrow.
base::ScopedCFTypeRef<CGPathRef> UnderlineStartPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathMoveToPoint(path, NULL, NSMidX(bounds) - 7, NSMidY(bounds) - 8);
  CGPathAddLineToPoint(path, NULL, NSMidX(bounds) + 7, NSMidY(bounds) - 8);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// 2. The same line, but above the arrow.
base::ScopedCFTypeRef<CGPathRef> UnderlineMidPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathMoveToPoint(path, NULL, NSMidX(bounds) - 7, NSMidY(bounds) + 7);
  CGPathAddLineToPoint(path, NULL, NSMidX(bounds) + 7, NSMidY(bounds) + 7);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// 3. A tiny arc, even higher, that serves as the progress stroke's flat "tail".
base::ScopedCFTypeRef<CGPathRef> UnderlineEndPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathAddArc(path, nullptr, NSMidX(bounds), NSMidY(bounds),
               NSWidth(bounds) / 2 - kStrokeWidth / 2, M_PI_2, M_PI * 0.45,
               true);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// The below shapes can all occupy a layer with these bounds.
constexpr CGRect kShapeBounds{{0, 0}, {14, 18}};

// The arrow part of the start state.
base::ScopedCFTypeRef<CGPathRef> ArrowPath() {
  constexpr CGPoint points[] = {
      {14, 7}, {10, 7}, {10, 13}, {4, 13}, {4, 7}, {0, 7}, {7, 0},
  };
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddLines(path, nullptr, points, sizeof(points) / sizeof(*points));
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

// ArrowPath() shifted slightly upward. Animating between two versions of the
// path was simpler than moving layers around.
base::ScopedCFTypeRef<CGPathRef> BeginArrowPath() {
  CGAffineTransform transform = CGAffineTransformMakeTranslation(0, 3);
  return base::ScopedCFTypeRef<CGPathRef>(
      CGPathCreateCopyByTransformingPath(ArrowPath(), &transform));
}

// A checkmark that represents a completed download. The number and order of
// points matches up with ArrowPath() so that one can morph into the other.
base::ScopedCFTypeRef<CGPathRef> CheckPath() {
  constexpr CGPoint points[] = {
      {11.1314, 6.5385}, {13.9697, 9.308}, {12.012, 11.366}, {4.5942, 3.9466},
      {2.0074, 6.528},   {0.0, 4.6687},    {4.5549, 0.0},
  };
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddLines(path, nullptr, points, sizeof(points) / sizeof(*points));
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

}  // namespace

@implementation MDDownloadItemProgressIndicator {
  CAShapeLayer* backgroundLayer_;
  CAShapeLayer* shapeLayer_;
  CAShapeLayer* strokeLayer_;
  CAShapeLayer* strokeTailLayer_;

  // The current and desired state.
  MDDownloadItemProgressIndicatorState state_;
  MDDownloadItemProgressIndicatorState targetState_;

  CGFloat progress_;

  // An array of blocks representing queued animations.
  base::scoped_nsobject<NSMutableArray<void (^)()>> animations_;
}

@synthesize paused = paused_;

- (void)setPaused:(BOOL)paused {
  if (paused_ == paused)
    return;
  paused_ = paused;
  if (paused_) {
    if (CAAnimation* indeterminateAnimation =
            [strokeLayer_ animationForKey:kIndeterminateAnimationKey]) {
      [strokeLayer_ setValue:[strokeLayer_.presentationLayer
                                 valueForKeyPath:@"transform.rotation.z"]
                  forKeyPath:@"transform.rotation.z"];
      [strokeLayer_ removeAnimationForKey:kIndeterminateAnimationKey];
    }
  } else {
    [strokeLayer_ setValue:@0 forKey:@"transform.rotation.z"];
    [self updateProgress];
  }
}

- (void)updateColors {
  CGColorRef indicatorColor =
      skia::SkColorToCalibratedNSColor(
          state_ == MDDownloadItemProgressIndicatorState::kComplete
              ? gfx::kGoogleGreen700
              : gfx::kGoogleBlue700)
          .CGColor;
  backgroundLayer_.fillColor = indicatorColor;
  shapeLayer_.fillColor =
      state_ == MDDownloadItemProgressIndicatorState::kInProgress
          ? skia::SkColorToCalibratedNSColor(gfx::kGoogleBlue500).CGColor
          : indicatorColor;
  strokeLayer_.strokeColor = indicatorColor;
  strokeTailLayer_.fillColor = indicatorColor;
}

- (void)setInProgressState {
  if (state_ == MDDownloadItemProgressIndicatorState::kInProgress)
    return;
  state_ = MDDownloadItemProgressIndicatorState::kInProgress;
  CALayer* layer = self.layer;

  // Give users a chance to see the starting state before it changes.
  const CFTimeInterval beginTime = CACurrentMediaTime() + 0.3;
  const auto delayAnimation = [&](CAAnimation* anim) {
    anim.beginTime = beginTime;
    anim.fillMode = kCAFillModeBackwards;
  };

  // This layer sits on top of shapeLayer_ to temporarily hide its new color.
  base::scoped_nsobject<CAShapeLayer> scopedBeginShapeLayer(
      [[CAShapeLayer alloc] init]);
  CAShapeLayer* beginShapeLayer = scopedBeginShapeLayer;
  beginShapeLayer.frame = shapeLayer_.frame;
  beginShapeLayer.path = shapeLayer_.path;
  beginShapeLayer.fillColor = shapeLayer_.fillColor;
  [layer insertSublayer:beginShapeLayer above:shapeLayer_];

  // As strokeTailLayer_, currently in the shape of a line under shapeLayer_,
  // sweeps upward, this mask follows it to reveal the new color.
  base::scoped_nsobject<CALayer> scopedMaskLayer([[CALayer alloc] init]);
  CALayer* maskLayer = scopedMaskLayer;

  // The final y position was chosen empirically so that the bottom edge of the
  // mask matches up with the line as it sweeps upward.
  maskLayer.frame =
      NSOffsetRect(beginShapeLayer.bounds, 0, NSHeight(layer.bounds) / 2 + 9);
  maskLayer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
  beginShapeLayer.mask = maskLayer;

  // The circular background of the progress indicator begins to slowly fade in.
  [backgroundLayer_ addAnimation:[&] {
    CABasicAnimation* anim = [CABasicAnimation animationWithKeyPath:@"opacity"];
    delayAnimation(anim);
    anim.fromValue = @(backgroundLayer_.opacity);
    anim.duration = 1;
    return anim;
  }()
                          forKey:nil];
  backgroundLayer_.opacity = 0.15;

  // Now, the line under the arrow sweeps upward to its final form: a small arc
  // segment that caps the progress stroke with a flat end.
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 2 / 3.0;
    // It accelerates with more "pep" than any standard MD curve.
    context.timingFunction = base::scoped_nsobject<CAMediaTimingFunction>(
        [[CAMediaTimingFunction alloc] initWithControlPoints:0.8:0:0.2:1]);

    [maskLayer addAnimation:[&] {
      CABasicAnimation* anim =
          [CABasicAnimation animationWithKeyPath:@"position.y"];
      delayAnimation(anim);
      anim.fromValue = @(NSMidY(beginShapeLayer.bounds));
      return anim;
    }()
                     forKey:nil];

    strokeTailLayer_.path = UnderlineEndPath(layer.bounds);
    [strokeTailLayer_ addAnimation:[&]() {
      CAKeyframeAnimation* anim =
          [CAKeyframeAnimation animationWithKeyPath:@"path"];
      delayAnimation(anim);
      anim.values = @[
        static_cast<id>(UnderlineStartPath(layer.bounds).get()),
        static_cast<id>(UnderlineMidPath(layer.bounds).get()),
        static_cast<id>(strokeTailLayer_.path),
      ];
      anim.keyTimes = @[ @0, @0.65, @1 ];
      return anim;
    }()
                            forKey:nil];

    CABasicAnimation* arrowAnim =
        [CABasicAnimation animationWithKeyPath:@"path"];
    delayAnimation(arrowAnim);
    arrowAnim.fromValue = static_cast<id>(beginShapeLayer.path);

    shapeLayer_.path = beginShapeLayer.path = ArrowPath();
    [beginShapeLayer addAnimation:arrowAnim forKey:nil];
    [shapeLayer_ addAnimation:arrowAnim forKey:nil];
    [self updateColors];
  }
      completionHandler:^{
        [beginShapeLayer removeFromSuperlayer];
      }];
}

- (void)setCompleteState {
  if (state_ == MDDownloadItemProgressIndicatorState::kComplete)
    return;
  state_ = MDDownloadItemProgressIndicatorState::kComplete;
  CABasicAnimation* shapeAnim = [CABasicAnimation animationWithKeyPath:@"path"];
  shapeAnim.fromValue = static_cast<id>(shapeLayer_.path);
  [shapeLayer_ addAnimation:shapeAnim forKey:nil];
  shapeLayer_.path = CheckPath();
  strokeLayer_.opacity = 0;
  [self updateColors];
}

- (void)setCanceledState {
  if (state_ == MDDownloadItemProgressIndicatorState::kCanceled)
    return;
  state_ = MDDownloadItemProgressIndicatorState::kCanceled;
  backgroundLayer_.opacity = 0;
  strokeLayer_.opacity = 0;
  [self updateColors];
}

- (void)updateProgress {
  NSAccessibilityPostNotification(self,
                                  NSAccessibilityValueChangedNotification);
  CGFloat newProgress = progress_;
  const BOOL isIndeterminate = newProgress < 0;
  if (isIndeterminate &&
      ![strokeLayer_ animationForKey:kIndeterminateAnimationKey]) {
    // Attach the indeterminate animation in its own transaction, so that it
    // doesn't delay any completion handlers indefinitely.
    [CATransaction setCompletionBlock:^{
      CABasicAnimation* anim =
          [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
      anim.byValue = @(M_PI * -2);
      anim.duration = kIndeterminateAnimationDuration;
      anim.repeatCount = HUGE_VALF;
      [strokeLayer_ addAnimation:anim forKey:kIndeterminateAnimationKey];
      [CATransaction flush];
    }];
  }
  if (!isIndeterminate) {
    // If the bar was in an indeterminate state, replace the continuous
    // rotation animation with a one-shot animation that resets it.
    if (CGFloat rotation =
            static_cast<NSNumber*>([strokeLayer_.presentationLayer
                                       valueForKeyPath:@"transform.rotation.z"])
                .doubleValue) {
      CABasicAnimation* anim =
          [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
      anim.fromValue = @(rotation);
      anim.byValue = @(-(rotation < 0 ? rotation : -(M_PI + rotation)));
      [strokeLayer_ addAnimation:anim forKey:kIndeterminateAnimationKey];
    } else {
      // …or, if the presentation layer wasn't rotated, just clear any
      // existing rotation animation.
      [strokeLayer_ removeAnimationForKey:kIndeterminateAnimationKey];
    }
  }
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    // The stroke moves just a hair faster than other elements.
    context.duration = 0.25;
    strokeLayer_.strokeEnd =
        isIndeterminate ? kIndeterminateArcSize : newProgress;
  }
                      completionHandler:nil];
}

- (void)applyStandardAnimationProperties:(NSAnimationContext*)context {
  context.duration = 0.3;
  context.timingFunction =
      CAMediaTimingFunction.cr_materialEaseInOutTimingFunction;
}

- (void)runAnimations {
  base::mac::ScopedBlock<void (^)()> anim([[animations_ firstObject] retain]);
  if (!anim) {
    animations_.reset();
    return;
  }
  [animations_ removeObjectAtIndex:0];
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 0.3;
    context.timingFunction =
        CAMediaTimingFunction.cr_materialEaseInOutTimingFunction;
    anim.get()();
  }
      completionHandler:^{
        [self runAnimations];
      }];
}

- (void)queueAnimation:(void (^)())block {
  if (animations_) {
    [animations_ addObject:[[block copy] autorelease]];
  } else {
    animations_.reset([[NSMutableArray alloc] init]);
    [animations_ addObject:[[block copy] autorelease]];
    if (self.layer)
      [self runAnimations];
  }
}

- (void)setState:(MDDownloadItemProgressIndicatorState)state
        progress:(CGFloat)progress
      animations:(void (^)())animations
      completion:(void (^)())completion {
  if (state != targetState_) {
    // The animation code is only prepared to move the state forward.
    DCHECK(state > targetState_);
    targetState_ = state;
    progress_ = progress;
    switch (targetState_) {
      case MDDownloadItemProgressIndicatorState::kNotStarted:
        NOTREACHED();
        break;
      case MDDownloadItemProgressIndicatorState::kInProgress:
        [self queueAnimation:^{
          [self setInProgressState];
        }];
        [self queueAnimation:^{
          [self updateProgress];
          if (animations)
            animations();
        }];
        break;
      case MDDownloadItemProgressIndicatorState::kComplete:
        [self queueAnimation:^{
          [self updateProgress];
        }];
        [self queueAnimation:^{
          [self setCompleteState];
          if (animations)
            animations();
        }];
        break;
      case MDDownloadItemProgressIndicatorState::kCanceled:
        [self queueAnimation:^{
          [self setCanceledState];
          [self updateProgress];
          if (animations)
            animations();
        }];
        break;
    }
  } else if (progress != progress_) {
    progress_ = progress;
    [self queueAnimation:^{
      [self updateProgress];
      if (animations)
        animations();
    }];
  } else if (animations) {
    animations();
  }

  if (completion)
    [self queueAnimation:completion];
}

// NSView overrides.

- (void)setLayer:(CALayer*)layer {
  super.layer = layer;

  base::scoped_nsobject<CAShapeLayer> backgroundLayer(
      [[CAShapeLayer alloc] init]);
  backgroundLayer_ = backgroundLayer;
  backgroundLayer_.frame = layer.bounds;
  backgroundLayer_.autoresizingMask =
      kCALayerWidthSizable | kCALayerHeightSizable;
  backgroundLayer_.opacity = 0;
  [layer addSublayer:backgroundLayer_];

  base::scoped_nsobject<CAShapeLayer> shapeLayer([[CAShapeLayer alloc] init]);
  shapeLayer_ = shapeLayer;
  shapeLayer_.bounds = kShapeBounds;
  shapeLayer_.path = BeginArrowPath();
  [layer addSublayer:shapeLayer_];

  base::scoped_nsobject<CAShapeLayer> strokeLayer([[CAShapeLayer alloc] init]);
  strokeLayer_ = strokeLayer;
  strokeLayer_.frame = layer.bounds;
  strokeLayer_.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
  strokeLayer_.fillColor = nil;
  strokeLayer_.lineWidth = kStrokeWidth;
  strokeLayer_.lineCap = kCALineCapRound;
  strokeLayer_.strokeStart = 0.02;
  strokeLayer_.strokeEnd = 0;
  [layer addSublayer:strokeLayer_];

  base::scoped_nsobject<CAShapeLayer> strokeTailLayer(
      [[CAShapeLayer alloc] init]);
  strokeTailLayer_ = strokeTailLayer;
  strokeTailLayer_.frame = layer.bounds;
  strokeTailLayer_.autoresizingMask =
      kCALayerWidthSizable | kCALayerHeightSizable;
  strokeTailLayer_.lineWidth = kStrokeWidth;
  [strokeLayer_ addSublayer:strokeTailLayer_];

  // If any state changes were queued up, kick 'em off.
  [self layout];
  [self runAnimations];
}

- (void)layout {
  backgroundLayer_.path = CirclePathWithBounds(self.layer.bounds);
  strokeLayer_.path = CirclePathWithBounds(
      CGRectInset(self.layer.bounds, kStrokeWidth / 2, kStrokeWidth / 2));
  shapeLayer_.position =
      CGPointMake(NSMidX(self.layer.bounds), NSMidY(self.layer.bounds) + 2);
  strokeTailLayer_.path =
      state_ == MDDownloadItemProgressIndicatorState::kNotStarted
          ? UnderlineStartPath(self.layer.bounds)
          : UnderlineEndPath(self.layer.bounds);
  strokeTailLayer_.position =
      CGPointMake(NSMidX(self.layer.bounds), NSMidY(self.layer.bounds));
  [self updateColors];
  [super layout];
}

// NSObject overrides.

- (BOOL)accessibilityIsIgnored {
  return NO;
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([NSAccessibilityRoleAttribute isEqualToString:attribute])
    return NSAccessibilityProgressIndicatorRole;
  if (progress_ >= 0) {
    if ([NSAccessibilityValueAttribute isEqualToString:attribute])
      return @(progress_);
    if ([NSAccessibilityMinValueAttribute isEqualToString:attribute])
      return @0;
    if ([NSAccessibilityMaxValueAttribute isEqualToString:attribute])
      return @1;
  }
  return [super accessibilityAttributeValue:attribute];
}

@end
