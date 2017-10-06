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

// 2. A narrower line above the arrow.
base::ScopedCFTypeRef<CGPathRef> UnderlineMidPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathMoveToPoint(path, NULL, NSMidX(bounds) - 7, NSMidY(bounds) + 7);
  CGPathAddLineToPoint(path, NULL, NSMidX(bounds) + 7, NSMidY(bounds) + 7);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// 3. A tiny arc segment that serves as the progress stroke's flat "tail".
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

// Currently the same as ArrowPath() shifted slightly upward. Animating between
// two versions of the path was simpler than moving layers around.
base::ScopedCFTypeRef<CGPathRef> BeginArrowPath() {
  constexpr CGPoint points[] = {
      {14, 10}, {10, 10}, {10, 16}, {4, 16}, {4, 10}, {0, 10}, {7, 3},
  };
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddLines(path, nullptr, points, sizeof(points) / sizeof(*points));
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

// The arrow part of the start state.
base::ScopedCFTypeRef<CGPathRef> ArrowPath() {
  constexpr CGPoint points[] = {
      {14, 7}, {10, 7}, {10, 13}, {4, 13}, {4, 7}, {0, 7}, {7, 0},
  };
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddLines(path, nullptr, points, sizeof(points) / sizeof(*points));
  return base::ScopedCFTypeRef<CGPathRef>(path);
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

  MDDownloadItemProgressIndicatorState state_;
  CGFloat progress_;

  // A block to run at the end of the current animation. It's also used to tell
  // if an animation is in progress, so could be set to an empty block.
  base::mac::ScopedBlock<void (^)()> completionHandler_;
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
          state_ == MDDownloadItemProgressIndicatorState::Complete
              ? gfx::kGoogleGreen700
              : gfx::kGoogleBlue700)
          .CGColor;
  backgroundLayer_.fillColor = indicatorColor;
  shapeLayer_.fillColor =
      state_ == MDDownloadItemProgressIndicatorState::InProgress
          ? skia::SkColorToCalibratedNSColor(gfx::kGoogleBlue500).CGColor
          : indicatorColor;
  strokeLayer_.strokeColor = indicatorColor;
  strokeTailLayer_.fillColor = indicatorColor;
}

- (void)setInProgressState {
  if (state_ == MDDownloadItemProgressIndicatorState::InProgress)
    return;
  state_ = MDDownloadItemProgressIndicatorState::InProgress;
  CALayer* layer = self.layer;

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
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 1;
    backgroundLayer_.opacity = 0.15;
  }
                      completionHandler:nil];

  // Now, the line under the arrow sweeps upward to its final form: a small arc
  // segment that caps the progress stroke with a flat end.
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 2 / 3.0;
    // It accelerates with more "pep" than any standard MD curve.
    context.timingFunction = base::scoped_nsobject<CAMediaTimingFunction>(
        [[CAMediaTimingFunction alloc] initWithControlPoints:0.8:0:0.2:1]);
    CABasicAnimation* maskAnim =
        [CABasicAnimation animationWithKeyPath:@"position.y"];
    maskAnim.fromValue = @(NSMidY(beginShapeLayer.bounds));
    [maskLayer addAnimation:maskAnim forKey:nil];

    strokeTailLayer_.path = UnderlineEndPath(layer.bounds);
    CAKeyframeAnimation* lineAnim =
        [CAKeyframeAnimation animationWithKeyPath:@"path"];
    lineAnim.values = @[
      static_cast<id>(UnderlineStartPath(layer.bounds).get()),
      static_cast<id>(UnderlineMidPath(layer.bounds).get()),
      static_cast<id>(strokeTailLayer_.path),
    ];
    lineAnim.keyTimes = @[ @0, @0.65, @1 ];
    [strokeTailLayer_ addAnimation:lineAnim forKey:nil];

    CABasicAnimation* arrowAnim =
        [CABasicAnimation animationWithKeyPath:@"path"];
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
  if (state_ == MDDownloadItemProgressIndicatorState::Complete)
    return;
  state_ = MDDownloadItemProgressIndicatorState::Complete;
  CABasicAnimation* shapeAnim = [CABasicAnimation animationWithKeyPath:@"path"];
  shapeAnim.fromValue = static_cast<id>(shapeLayer_.path);
  [shapeLayer_ addAnimation:shapeAnim forKey:nil];
  shapeLayer_.path = CheckPath();
  strokeLayer_.opacity = 0;
  [self updateColors];
}

- (void)setCanceledState {
  if (state_ == MDDownloadItemProgressIndicatorState::Canceled)
    return;
  state_ = MDDownloadItemProgressIndicatorState::Canceled;
  backgroundLayer_.opacity = 0;
  strokeLayer_.opacity = 0;
  [self updateColors];
}

- (void)updateProgress {
  const BOOL isIndeterminate = progress_ < 0;
  if (isIndeterminate &&
      ![strokeLayer_ animationForKey:kIndeterminateAnimationKey]) {
    [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
      context.timingFunction = nil;
      CABasicAnimation* anim =
          [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
      anim.byValue = @(M_PI * -2);
      anim.duration = kIndeterminateAnimationDuration;
      anim.repeatCount = HUGE_VALF;
      [strokeLayer_ addAnimation:anim forKey:kIndeterminateAnimationKey];
    }
                        completionHandler:nil];
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
        isIndeterminate ? kIndeterminateArcSize : progress_;
  }
                      completionHandler:nil];
}

- (void)applyStandardAnimationProperties:(NSAnimationContext*)context {
  context.duration = 0.3;
  context.timingFunction =
      CAMediaTimingFunction.cr_materialEaseInOutTimingFunction;
}

- (void)addCompletionHandler:(void (^)())block {
  if (!block) {
    // If the caller passed nil, a completion handler should still be set so
    // that further state changes know to queue up behind it.
    if (!completionHandler_)
      completionHandler_.reset([^{
      } copy]);
  } else if (auto prevCompletionHandler = std::move(completionHandler_)) {
    completionHandler_.reset([^{
      prevCompletionHandler.get()();
      block();
    } copy]);
  } else {
    completionHandler_.reset([block copy]);
  }
}

- (void)runCompletionHandlers {
  if (auto completionHandler = std::move(completionHandler_))
    completionHandler.get()();
}

- (void)setState:(MDDownloadItemProgressIndicatorState)state
        progress:(CGFloat)progress
      animations:(void (^)())animations
      completion:(void (^)())completion {
  // If the view hasn't been inserted into a hierarchy yet, or if a state
  // transition is already in progress, queue this change for later.
  if (!self.layer || completionHandler_) {
    [self addCompletionHandler:^{
      [self setState:state
            progress:progress
          animations:animations
          completion:completion];
    }];
    return;
  }

  // No matter how it's added to a transaction, a completion handler must be
  // added before any animations. This variable lets a decision to run
  // completion handlers as part of this transaction be changed later.
  __block BOOL runCompletionHandlersAfterAnimation = YES;

  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    [self applyStandardAnimationProperties:context];

    if (state_ != state) {
      // This code is only prepared to move the state forward.
      DCHECK(state >= state_);
      switch (state) {
        case MDDownloadItemProgressIndicatorState::NotStarted:
          NOTREACHED();
          break;
        case MDDownloadItemProgressIndicatorState::InProgress:
          // Give folks a chance to get comfortable with the starting state
          // before launching any animations.
          dispatch_after(
              dispatch_time(DISPATCH_TIME_NOW, 0.3 * NSEC_PER_SEC),
              dispatch_get_main_queue(), ^{
                [NSAnimationContext
                    runAnimationGroup:^(NSAnimationContext* context) {
                      [self applyStandardAnimationProperties:context];
                      [self setInProgressState];
                      if (animations)
                        animations();
                    }
                    completionHandler:^{
                      [self runCompletionHandlers];
                    }];
              });
          // Wait to set the progress until after the starting animation.
          [self addCompletionHandler:^{
            [self setState:state
                  progress:progress
                animations:nil
                completion:completion];
          }];
          // Changes will queue until the above call to -runCompletionHandlers.
          runCompletionHandlersAfterAnimation = NO;
          return;
          break;
        case MDDownloadItemProgressIndicatorState::Complete:
          // Make sure progress reaches 1 before kicking off the animation.
          [self setState:state_
                progress:1
              animations:nil
              completion:^{
                [NSAnimationContext
                    runAnimationGroup:^(NSAnimationContext* context) {
                      [self applyStandardAnimationProperties:context];
                      [self setCompleteState];
                      if (animations)
                        animations();
                      [self addCompletionHandler:completion];
                    }
                    completionHandler:^{
                      [self runCompletionHandlers];
                    }];
              }];
          // Control is with the reentrant -setState:…, now.
          runCompletionHandlersAfterAnimation = NO;
          return;
          break;
        case MDDownloadItemProgressIndicatorState::Canceled:
          [self setCanceledState];
          break;
      }
    }

    if (progress != progress_) {
      progress_ = progress;
      [self updateProgress];
      NSAccessibilityPostNotification(self,
                                      NSAccessibilityValueChangedNotification);

      // The indeterminate animation repeats forever, so don't wait for it.
      if (progress < 0)
        runCompletionHandlersAfterAnimation = NO;
    }

    // Run any companion animations.
    if (animations)
      animations();

    if (runCompletionHandlersAfterAnimation) {
      [self addCompletionHandler:completion];
    } else {
      [self runCompletionHandlers];
      if (completion)
        completion();
    }
  }
      completionHandler:^{
        if (runCompletionHandlersAfterAnimation)
          [self runCompletionHandlers];
      }];
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
  [self runCompletionHandlers];
}

- (void)layout {
  backgroundLayer_.path = CirclePathWithBounds(self.layer.bounds);
  strokeLayer_.path = CirclePathWithBounds(
      CGRectInset(self.layer.bounds, kStrokeWidth / 2, kStrokeWidth / 2));
  shapeLayer_.position =
      CGPointMake(NSMidX(self.layer.bounds), NSMidY(self.layer.bounds) + 2);
  strokeTailLayer_.path =
      state_ == MDDownloadItemProgressIndicatorState::NotStarted
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
