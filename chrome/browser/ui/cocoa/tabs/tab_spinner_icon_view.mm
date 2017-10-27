// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_spinner_icon_view.h"

#include "base/mac/scoped_cftyperef.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/grit/theme_resources.h"
#include "components/grit/components_scaled_resources.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/native_theme/native_theme.h"

namespace {
const CGFloat kDegrees90 = gfx::DegToRad(90.0f);
const CGFloat kDegrees180 = gfx::DegToRad(180.0f);
const CGFloat kDegrees270 = gfx::DegToRad(270.0f);
const CGFloat kDegrees360 = gfx::DegToRad(360.0f);
const CGFloat kReverseArcStartAngle = kDegrees270;
const CGFloat kReverseArcEndAngle = (kReverseArcStartAngle + kDegrees180);
const CGFloat kWaitingStrokeAlpha = 0.5;
const CGFloat kSadTabAnimationTime = 0.5;
}  // namespace

@implementation TabSpinnerIconView : SpinnerView

@synthesize tabLoadingState = tabLoadingState_;

+ (CGFloat)reverseArcLength {
  static CGFloat reverseArcLength = kDegrees90 * [self spinnerArcRadius] * 2;
  return reverseArcLength;
}

- (void)viewDidMoveToWindow {
  if ([self window]) {
    [self updateSpinnerColor];
  }
  [super viewDidMoveToWindow];
}

- (BOOL)showingAnIcon {
  return tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone;
}

- (NSColor*)spinnerColor {
  if ([self showingAnIcon]) {
    return [NSColor clearColor];
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
      return [[NSColor whiteColor] colorWithAlphaComponent:kWaitingStrokeAlpha];
    }

    if (theme) {
      return theme->GetNSColor(ThemeProperties::COLOR_TAB_THROBBER_WAITING);
    }

    SkColor skWaitingColor =
        ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
            ui::NativeTheme::kColorId_ThrobberWaitingColor);
    return skia::SkColorToSRGBNSColor(skWaitingColor);
  }

  if (hasDarkTheme) {
    return [NSColor whiteColor];
  }

  if (theme) {
    return theme->GetNSColor(ThemeProperties::COLOR_TAB_THROBBER_SPINNING);
  }

  return [NSColor redColor];
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
  }
  if (tabLoadingState_ == kTabCrashed) {
    return [self initializeToastAnimation];
  }
  if (tabLoadingState_ != kTabWaiting) {
    return;
  }

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
  [arcAnimation
      setTimingFunction:[CAMediaTimingFunction
                            functionWithName:kCAMediaTimingFunctionLinear]];
  [arcAnimation setKeyPath:@"lineDashPhase"];
  CGFloat scaleFactor = [self scaleFactor];
  NSArray* animationValues =
      @[ @(-[TabSpinnerIconView reverseArcLength] * scaleFactor), @(0.0) ];
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
  [group setAnimations:@[ arcAnimation ]];
  spinnerAnimation_.reset([group retain]);

  // Finally, create an animation that rotates the entire spinner layer.
  CABasicAnimation* rotationAnimation = [CABasicAnimation animation];
  rotationAnimation.keyPath = @"transform.rotation";
  [rotationAnimation setFromValue:@0];
  [rotationAnimation setToValue:@(kDegrees360)];
  // Start the rotation once the stroke animation has completed.
  [rotationAnimation
      setBeginTime:CACurrentMediaTime() + reverseArcAnimationTime];
  [rotationAnimation setDuration:[TabSpinnerIconView rotationTime]];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];

  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)configureShapeLayerPathAndPattern {
  [shapeLayer_ setContents:nil];

  if (tabLoadingState_ == kTabWaiting) {
    CGRect bounds = [self bounds];
    CGFloat scaleFactor = [self scaleFactor];
    CGFloat arcRadius = [TabSpinnerIconView spinnerArcRadius] * scaleFactor;

    base::ScopedCFTypeRef<CGMutablePathRef> shapePath(CGPathCreateMutable());

    [shapeLayer_ setLineDashPattern:@[ @([TabSpinnerIconView reverseArcLength] *
                                         scaleFactor) ]];
    CGPathAddArc(shapePath, NULL, bounds.size.width / 2.0,
                 bounds.size.height / 2.0, arcRadius, kReverseArcStartAngle,
                 kReverseArcEndAngle, 1);
    [shapeLayer_ setPath:shapePath];
  } else if (tabLoadingState_ == kTabLoading) {
    [super configureShapeLayerPathAndPattern];
  }
}

- (void)setTabLoadingState:(TabLoadingState)loadingState icon:(NSImage*)icon {
  tabLoadingState_ = loadingState;

  if (tabLoadingState_ == kTabWaiting || tabLoadingState_ == kTabLoading) {
    [self configureShapeLayerPathAndPattern];
  } else {
    [shapeLayer_ setPath:nullptr];
    [shapeLayer_ setContents:icon];
  }

  spinnerAnimation_.reset();
  rotationAnimation_.reset();
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];

  [self updateSpinnerColor];
  [self updateAnimation:nil];
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState {
  if (tabLoadingState_ == newLoadingState) {
    return;
  }

  NSImage* tabIcon = nil;
  if (newLoadingState == kTabCrashed) {
    static NSImage* sadTabIcon = ResourceBundle::GetSharedInstance()
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
  [self updateSpinnerColor];
}

- (void)windowDidChangeActive {
}

@end
