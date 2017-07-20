// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/spinner_view.h"

#import <QuartzCore/QuartzCore.h>

#include "base/mac/scoped_cftyperef.h"
#import "chrome/browser/themes/theme_properties.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/theme_provider.h"
#include "ui/native_theme/native_theme.h"

namespace {
const CGFloat kDegrees90               = (M_PI / 2);
const CGFloat kDegrees180              = (M_PI);
const CGFloat kDegrees270              = (3 * M_PI / 2);
const CGFloat kDegrees360              = (2 * M_PI);
const CGFloat kDesignWidth             = 28.0;
const CGFloat kArcRadius               = 12.5;
const CGFloat kArcDiameter             = kArcRadius * 2.0;
const CGFloat kArcLength               = 58.9;
const CGFloat kArcStrokeWidth          = 3.0;
const CGFloat kArcAnimationTime        = 1.333;
const CGFloat kArcStartAngle           = kDegrees180;
const CGFloat kArcEndAngle             = (kArcStartAngle + kDegrees270);
const CGFloat kRotationTime            = 1.56863;
NSString* const kSpinnerAnimationName  = @"SpinnerAnimationName";
NSString* const kRotationAnimationName = @"RotationAnimationName";
}

@implementation SpinnerView

+ (CGFloat)rotationTime {
  return kRotationTime;
}

- (instancetype)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {
    [self setWantsLayer:YES];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

// Register/unregister for window miniaturization event notifications so that
// the spinner can stop animating if the window is minaturized
// (i.e. not visible).
- (void)viewWillMoveToWindow:(NSWindow*)newWindow {
  if ([self window]) {
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
                  name:NSWindowWillMiniaturizeNotification
                object:[self window]];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
                  name:NSWindowDidDeminiaturizeNotification
                object:[self window]];
  }

  if (newWindow) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(updateAnimation:)
               name:NSWindowWillMiniaturizeNotification
             object:newWindow];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(updateAnimation:)
               name:NSWindowDidDeminiaturizeNotification
             object:newWindow];
  }
}

// Start or stop the animation whenever the view is added to or removed from a
// window.
- (void)viewDidMoveToWindow {
  [self updateAnimation:nil];
}

- (BOOL)isAnimating {
  return [shapeLayer_ animationForKey:kSpinnerAnimationName] != nil ||
      [rotationLayer_ animationForKey:kRotationAnimationName] != nil;
}

- (void)updateShapeLayerStrokeColor {
  SkColor skStrokeColor =
      ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
          ui::NativeTheme::kColorId_ThrobberSpinningColor);
  base::ScopedCFTypeRef<CGColorRef> strokeColor(
      skia::CGColorCreateFromSkColor(skStrokeColor));
  [shapeLayer_ setStrokeColor:strokeColor];
}

// Overridden to return a custom CALayer for the view (called from
// setWantsLayer:).
- (CALayer*)makeBackingLayer {
  CGRect bounds = [self bounds];
  // The spinner was designed to be |kDesignWidth| points wide. Compute the
  // scale factor needed to scale design parameters like |RADIUS| so that the
  // spinner scales to fit the view's bounds.
  CGFloat scaleFactor = bounds.size.width / kDesignWidth;

  shapeLayer_ = [CAShapeLayer layer];
  [shapeLayer_ setDelegate:self];
  [shapeLayer_ setBounds:bounds];
  // Per the design, the line width does not scale linearly.
  CGFloat scaledDiameter = kArcDiameter * scaleFactor;
  CGFloat lineWidth;
  if (scaledDiameter < kArcDiameter) {
    lineWidth = kArcStrokeWidth - (kArcDiameter - scaledDiameter) / 16.0;
  } else {
    lineWidth = kArcStrokeWidth + (scaledDiameter - kArcDiameter) / 11.0;
  }
  [shapeLayer_ setLineWidth:lineWidth];
  [shapeLayer_ setLineCap:kCALineCapRound];
  [shapeLayer_ setLineDashPattern:@[ @(kArcLength * scaleFactor) ]];
  [shapeLayer_ setFillColor:NULL];
  [self updateShapeLayerStrokeColor];

  // Create the arc that, when stroked, creates the spinner.
  base::ScopedCFTypeRef<CGMutablePathRef> shapePath(CGPathCreateMutable());
  CGPathAddArc(shapePath, NULL, bounds.size.width / 2.0,
               bounds.size.height / 2.0, kArcRadius * scaleFactor,
               kArcStartAngle, kArcEndAngle, 0);
  [shapeLayer_ setPath:shapePath];

  // Place |shapeLayer_| in a layer so that it's easy to rotate the entire
  // spinner animation.
  rotationLayer_ = [CALayer layer];
  [rotationLayer_ setBounds:bounds];
  [rotationLayer_ addSublayer:shapeLayer_];
  [shapeLayer_ setPosition:CGPointMake(NSMidX(bounds), NSMidY(bounds))];

  // Place |rotationLayer_| in a parent layer so that it's easy to rotate
  // |rotationLayer_| around the center of the view.
  CALayer* parentLayer = [CALayer layer];
  [parentLayer setBounds:bounds];
  [parentLayer addSublayer:rotationLayer_];
  [rotationLayer_ setPosition:CGPointMake(bounds.size.width / 2.0,
                                          bounds.size.height / 2.0)];
  return parentLayer;
}

// Overridden to start or stop the animation whenever the view is unhidden or
// hidden.
- (void)setHidden:(BOOL)flag {
  [super setHidden:flag];
  [self updateAnimation:nil];
}

// Make sure the layer's backing store matches the window as the window moves
// between screens.
- (BOOL)layer:(CALayer*)layer
    shouldInheritContentsScale:(CGFloat)newScale
                    fromWindow:(NSWindow*)window {
  return YES;
}

// The spinner animation consists of four cycles that it continuously repeats.
// Each cycle consists of one complete rotation of the spinner's arc plus a
// rotation adjustment at the end of each cycle (see rotation animation comment
// below for the reason for the adjustment). The arc's length also grows and
// shrinks over the course of each cycle, which the spinner achieves by drawing
// the arc using a (solid) dashed line pattern and animating the "lineDashPhase"
// property.
- (void)initializeAnimation {
  CGRect bounds = [self bounds];
  CGFloat scaleFactor = bounds.size.width / kDesignWidth;

  // Make sure |shapeLayer_|'s content scale factor matches the window's
  // backing depth (e.g. it's 2.0 on Retina Macs). Don't worry about adjusting
  // any other layers because |shapeLayer_| is the only one displaying content.
  CGFloat backingScaleFactor = [[self window] backingScaleFactor];
  [shapeLayer_ setContentsScale:backingScaleFactor];

  // Create the first half of the arc animation, where it grows from a short
  // block to its full length.
  base::scoped_nsobject<CAMediaTimingFunction> timingFunction(
      [[CAMediaTimingFunction alloc] initWithControlPoints:0.4 :0.0 :0.2 :1]);
  base::scoped_nsobject<CAKeyframeAnimation> firstHalfAnimation(
      [[CAKeyframeAnimation alloc] init]);
  [firstHalfAnimation setTimingFunction:timingFunction];
  [firstHalfAnimation setKeyPath:@"lineDashPhase"];
  // Begin the lineDashPhase animation just short of the full arc length,
  // otherwise the arc will be zero length at start.
  NSArray* animationValues = @[ @(-(kArcLength - 0.4) * scaleFactor), @(0.0) ];
  [firstHalfAnimation setValues:animationValues];
  NSArray* keyTimes = @[ @(0.0), @(1.0) ];
  [firstHalfAnimation setKeyTimes:keyTimes];
  [firstHalfAnimation setDuration:kArcAnimationTime / 2.0];
  [firstHalfAnimation setRemovedOnCompletion:NO];
  [firstHalfAnimation setFillMode:kCAFillModeForwards];

  // Create the second half of the arc animation, where it shrinks from full
  // length back to a short block.
  base::scoped_nsobject<CAKeyframeAnimation> secondHalfAnimation(
      [[CAKeyframeAnimation alloc] init]);
  [secondHalfAnimation setTimingFunction:timingFunction];
  [secondHalfAnimation setKeyPath:@"lineDashPhase"];
  // Stop the lineDashPhase animation just before it reaches the full arc
  // length, otherwise the arc will be zero length at the end.
  animationValues = @[ @(0.0), @((kArcLength - 0.3) * scaleFactor) ];
  [secondHalfAnimation setValues:animationValues];
  [secondHalfAnimation setKeyTimes:keyTimes];
  [secondHalfAnimation setDuration:kArcAnimationTime / 2.0];
  [secondHalfAnimation setRemovedOnCompletion:NO];
  [secondHalfAnimation setFillMode:kCAFillModeForwards];

  // Make four copies of the arc animations, to cover the four complete cycles
  // of the full animation.
  NSMutableArray* animations = [NSMutableArray array];
  CGFloat beginTime = 0;
  for (NSUInteger i = 0; i < 4; i++, beginTime += kArcAnimationTime) {
    [firstHalfAnimation setBeginTime:beginTime];
    [secondHalfAnimation setBeginTime:beginTime + kArcAnimationTime / 2.0];
    [animations addObject:firstHalfAnimation];
    [animations addObject:secondHalfAnimation];
    firstHalfAnimation.reset([firstHalfAnimation copy]);
    secondHalfAnimation.reset([secondHalfAnimation copy]);
  }

  // Create a step rotation animation, which rotates the arc 90 degrees on each
  // cycle. Each arc starts as a short block at degree 0 and ends as a short
  // block at degree -270. Without a 90 degree rotation at the end of each
  // cycle, the short block would appear to suddenly jump from -270 degrees to
  // -360 degrees. The full animation has to contain four of these 90 degree
  // adjustments in order for the arc to return to its starting point, at which
  // point the full animation can smoothly repeat.
  CAKeyframeAnimation* stepRotationAnimation = [CAKeyframeAnimation animation];
  [stepRotationAnimation setTimingFunction:
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear]];
  [stepRotationAnimation setKeyPath:@"transform.rotation"];
  animationValues = @[ @(0.0), @(0.0),
                       @(kDegrees90),
                       @(kDegrees90),
                       @(kDegrees180),
                       @(kDegrees180),
                       @(kDegrees270),
                       @(kDegrees270)];
  [stepRotationAnimation setValues:animationValues];
  keyTimes = @[ @(0.0), @(0.25), @(0.25), @(0.5), @(0.5), @(0.75), @(0.75),
                @(1.0) ];
  [stepRotationAnimation setKeyTimes:keyTimes];
  [stepRotationAnimation setDuration:kArcAnimationTime * 4.0];
  [stepRotationAnimation setRemovedOnCompletion:NO];
  [stepRotationAnimation setFillMode:kCAFillModeForwards];
  [stepRotationAnimation setRepeatCount:HUGE_VALF];
  [animations addObject:stepRotationAnimation];

  // Use an animation group so that the animations are easier to manage, and to
  // give them the best chance of firing synchronously.
  CAAnimationGroup* group = [CAAnimationGroup animation];
  [group setDuration:kArcAnimationTime * 4];
  [group setRepeatCount:HUGE_VALF];
  [group setFillMode:kCAFillModeForwards];
  [group setRemovedOnCompletion:NO];
  [group setAnimations:animations];

  spinnerAnimation_.reset([group retain]);

  // Finally, create an animation that rotates the entire spinner layer.
  CABasicAnimation* rotationAnimation = [CABasicAnimation animation];
  rotationAnimation.keyPath = @"transform.rotation";
  [rotationAnimation setFromValue:@0];
  [rotationAnimation setToValue:@(-kDegrees360)];
  [rotationAnimation setDuration:kRotationTime];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];

  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)updateAnimation:(NSNotification*)notification {
  // Only animate the spinner if it's within a window, and that window is not
  // currently minimized or being minimized.
  if ([self window] && ![[self window] isMiniaturized] && ![self isHidden] &&
      ![[notification name] isEqualToString:
           NSWindowWillMiniaturizeNotification]) {
    if (spinnerAnimation_.get() == nil) {
      [self initializeAnimation];
    }
    if (![self isAnimating]) {
      [shapeLayer_ addAnimation:spinnerAnimation_.get()
                         forKey:kSpinnerAnimationName];
      [rotationLayer_ addAnimation:rotationAnimation_.get()
                            forKey:kRotationAnimationName];
    }
  } else {
    [shapeLayer_ removeAllAnimations];
    [rotationLayer_ removeAllAnimations];
  }
}

@end

#include "chrome/grit/theme_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace {
const CGFloat kReverseArcLength        = 39.25;
const CGFloat kReverseArcStartAngle    = kDegrees270;
const CGFloat kReverseArcEndAngle      = (kReverseArcStartAngle + kDegrees180);
const CGFloat kWaitingStrokeAlpha      = 0.5;
const CGFloat kSadTabAnimationTime     = 0.5;
}  // namespace

@implementation TabSpinnerIconView : SpinnerView

@synthesize tabLoadingState = tabLoadingState_;

- (void)viewDidMoveToWindow {
  if ([self window]) {
    [self updateShapeLayerStrokeColor];
  }
  [super viewDidMoveToWindow];
}

- (BOOL)showingAnIcon {
  return tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone;
}

- (void)updateShapeLayerStrokeColor {
  if ([self showingAnIcon]) {
    [shapeLayer_ setStrokeColor:[NSColor clearColor].CGColor];
    return;
  }

  BOOL hasDarkTheme =
      [[self window] respondsToSelector:@selector(hasDarkTheme)] &&
      [[self window] hasDarkTheme];
  const ui::ThemeProvider* theme = nullptr;
  if ([[self window] respondsToSelector:@selector(themeProvider)]) {
    theme = [[self window] themeProvider];
  }

  if (tabLoadingState_ == kTabWaiting) {
    if (hasDarkTheme) {
      [shapeLayer_ setStrokeColor:
          [[NSColor whiteColor] colorWithAlphaComponent:kWaitingStrokeAlpha].CGColor];
    } else {
      if (theme) {
        [shapeLayer_ setStrokeColor:
            theme->GetNSColor(ThemeProperties::COLOR_TAB_THROBBER_WAITING).CGColor];
      } else {
        SkColor skWaitingColor =
            ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
                ui::NativeTheme::kColorId_ThrobberWaitingColor);
        base::ScopedCFTypeRef<CGColorRef> waitingColor(
            skia::CGColorCreateFromSkColor(skWaitingColor));
        [shapeLayer_ setStrokeColor:waitingColor];
      }
    }
    return;
  }

  if (hasDarkTheme) {
    [shapeLayer_ setStrokeColor:[NSColor whiteColor].CGColor];
  } else {
    CGColorRef spinningColor = [NSColor redColor].CGColor;
    if (theme) {
      spinningColor =
          theme->GetNSColor(ThemeProperties::COLOR_TAB_THROBBER_SPINNING).CGColor;
    } else {
      [super updateShapeLayerStrokeColor];
    }
  }
}

- (void)initializeToastAnimation {
  const CGFloat offset = [self bounds].size.height;

  CABasicAnimation* rotationAnimation = [CABasicAnimation animation];
  rotationAnimation.keyPath = @"transform.translation.y";
  [rotationAnimation setFromValue:@(-offset)];
  [rotationAnimation setToValue:@(0)];
  [rotationAnimation setDuration:kSadTabAnimationTime];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:0];

  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)initializeAnimation {
  if (tabLoadingState_ == kTabLoading) {
    return [super initializeAnimation];
  } else if (tabLoadingState_ == kTabCrashed) {
    return [self initializeToastAnimation];
  } else if (tabLoadingState_ != kTabWaiting) {
    return;
  }

  CGRect bounds = [self bounds];
  CGFloat scaleFactor = bounds.size.width / kDesignWidth;

  // Make sure |shapeLayer_|'s content scale factor matches the window's
  // backing depth (e.g. it's 2.0 on Retina Macs). Don't worry about adjusting
  // any other layers because |shapeLayer_| is the only one displaying content.
  CGFloat backingScaleFactor = [[self window] backingScaleFactor];
  [shapeLayer_ setContentsScale:backingScaleFactor];
  const CGFloat reverseArcAnimationTime = [SpinnerView rotationTime] / 2.0;

  // Create the arc animation, where it grows from a short block to its full
  // length.
  base::scoped_nsobject<CAKeyframeAnimation> arcAnimation(
      [[CAKeyframeAnimation alloc] init]);
  [arcAnimation setTimingFunction:
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear]];
  [arcAnimation setKeyPath:@"lineDashPhase"];
  NSArray* animationValues = @[ @(-kReverseArcLength * scaleFactor), @(0.0) ];
  [arcAnimation setValues:animationValues];
  NSArray* keyTimes = @[ @(0.0), @(1.0) ];
  [arcAnimation setKeyTimes:keyTimes];
  [arcAnimation setDuration:reverseArcAnimationTime];
  [arcAnimation setRemovedOnCompletion:NO];
  [arcAnimation setFillMode:kCAFillModeForwards];

  CAAnimationGroup* group = [CAAnimationGroup animation];
  [group setDuration:reverseArcAnimationTime];
  [group setFillMode:kCAFillModeForwards];
  [group setRemovedOnCompletion:NO];
  [group setAnimations:@[arcAnimation]];
  spinnerAnimation_.reset([group retain]);

  // Finally, create an animation that rotates the entire spinner layer.
  CABasicAnimation* rotationAnimation = [CABasicAnimation animation];
  rotationAnimation.keyPath = @"transform.rotation";
  [rotationAnimation setFromValue:@0];
  [rotationAnimation setToValue:@(kDegrees360)];
  // Start the rotation once the stroke animation has completed.
  [rotationAnimation setBeginTime:CACurrentMediaTime() + reverseArcAnimationTime];
  [rotationAnimation setDuration:kRotationTime];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];
  
  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)setTabLoadingState:(TabLoadingState)loadingState icon:(NSImage*)icon {
  tabLoadingState_ = loadingState;
  [shapeLayer_ setContents:nil];

  CGRect bounds = [self bounds];
  CGFloat scaleFactor = bounds.size.width / kDesignWidth;
  base::ScopedCFTypeRef<CGMutablePathRef> shapePath(CGPathCreateMutable());
  if (tabLoadingState_ == kTabWaiting) {
    [shapeLayer_ setLineDashPattern:@[ @(kReverseArcLength * scaleFactor) ]];
    CGPathAddArc(shapePath, NULL, bounds.size.width / 2.0,
                 bounds.size.height / 2.0, kArcRadius * scaleFactor,
                 kReverseArcStartAngle, kReverseArcEndAngle, 1);
    [shapeLayer_ setPath:shapePath];
  } else if (tabLoadingState_ == kTabLoading) {
    [shapeLayer_ setLineDashPattern:@[ @(kArcLength * scaleFactor) ]];
    CGPathAddArc(shapePath, NULL, bounds.size.width / 2.0,
                 bounds.size.height / 2.0, kArcRadius * scaleFactor,
                 kArcStartAngle, kArcEndAngle, 0);
    [shapeLayer_ setPath:shapePath];
  } else {
    [shapeLayer_ setPath:nullptr];
    [shapeLayer_ setContents:icon];
  }
  spinnerAnimation_.reset();
  rotationAnimation_.reset();
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];

  [self updateShapeLayerStrokeColor];
  [self updateAnimation:nil];
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState {
  if (tabLoadingState_ == newLoadingState) {
    return;
  }

  NSImage* tabIcon = nil;
  if (newLoadingState == kTabCrashed) {
    static NSImage* sadTabIcon =
        ResourceBundle::GetSharedInstance()
        .GetNativeImageNamed(IDR_CRASH_SAD_FAVICON)
        .CopyNSImage();
    tabIcon = sadTabIcon;
  }

  [self setTabLoadingState:newLoadingState icon:tabIcon];
}

- (void)setTabDoneIcon:(NSImage*)anImage {
  [self setTabLoadingState:kTabDone icon:anImage];
}

- (void)updateAnimation:(NSNotification*)notification {
  if (tabLoadingState_ == kTabDone) {
    [shapeLayer_ removeAllAnimations];
    [rotationLayer_ removeAllAnimations];
  } else {
    [super updateAnimation:notification];
  }
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self updateShapeLayerStrokeColor];
}

- (void)windowDidChangeActive {
}

@end
