// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_spinner_view.h"

#include "base/mac/scoped_cftyperef.h"
#include "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/native_theme/native_theme.h"

namespace {
constexpr CGFloat kDegrees90 = gfx::DegToRad(90.0f);
constexpr CGFloat kDegrees180 = gfx::DegToRad(180.0f);
constexpr CGFloat kDegrees270 = gfx::DegToRad(270.0f);
constexpr CGFloat kDegrees360 = gfx::DegToRad(360.0f);
constexpr CGFloat kWaitingStrokeAlpha = 0.5;
}  // namespace

@interface TabSpinnerView () {
  BOOL spinReverse_;
}
@end

@implementation TabSpinnerView

- (NSColor*)spinnerColor {
  BOOL hasDarkTheme =
      [[self window] respondsToSelector:@selector(hasDarkTheme)] &&
      [[self window] hasDarkTheme];

  if (hasDarkTheme) {
    return spinReverse_ ? [[NSColor whiteColor]
                              colorWithAlphaComponent:kWaitingStrokeAlpha]
                        : [NSColor whiteColor];
  }

  const ui::ThemeProvider* theme = nullptr;
  if ([[self window] respondsToSelector:@selector(themeProvider)]) {
    theme = [[self window] themeProvider];
  }

  if (spinReverse_) {
    if (theme) {
      return theme->GetNSColor(ThemeProperties::COLOR_TAB_THROBBER_WAITING);
    }

    SkColor skWaitingColor =
        ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
            ui::NativeTheme::kColorId_ThrobberWaitingColor);
    return skia::SkColorToSRGBNSColor(skWaitingColor);
  }

  return theme ? theme->GetNSColor(ThemeProperties::COLOR_TAB_THROBBER_SPINNING)
               : [super spinnerColor];
}

- (CGFloat)arcStartAngle {
  return spinReverse_ ? kDegrees270 : [super arcStartAngle];
}

- (CGFloat)arcEndAngleDelta {
  return spinReverse_ ? kDegrees180 : [super arcEndAngleDelta];
}

- (CGFloat)arcLength {
  // The reverse arc spans 90 degrees of circumference.
  static CGFloat reverseArcLength =
      kDegrees90 * ([SpinnerView arcUnitRadius] * 2);

  return spinReverse_ ? reverseArcLength : [super arcLength];
}

- (void)initializeAnimation {
  if (!spinReverse_) {
    return [super initializeAnimation];
  }

  const CGFloat forwardArcRotationTime = [SpinnerView arcRotationTime];
  const CGFloat reverseArcAnimationTime = forwardArcRotationTime / 2.0;

  // Create the arc animation.
  base::scoped_nsobject<CAKeyframeAnimation> arcAnimation(
      [[CAKeyframeAnimation alloc] init]);
  [arcAnimation
      setTimingFunction:[CAMediaTimingFunction
                            functionWithName:kCAMediaTimingFunctionLinear]];
  [arcAnimation setKeyPath:@"lineDashPhase"];
  CGFloat scaleFactor = [self scaleFactor];
  NSArray* animationValues = @[ @(-[self arcLength] * scaleFactor), @(0.0) ];
  [arcAnimation setValues:animationValues];
  NSArray* keyTimes = @[ @(0.0), @(1.0) ];
  [arcAnimation setKeyTimes:keyTimes];
  [arcAnimation setDuration:reverseArcAnimationTime];
  [arcAnimation setFillMode:kCAFillModeForwards];

  CAAnimationGroup* group = [CAAnimationGroup animation];
  [group setDuration:reverseArcAnimationTime];
  [group setFillMode:kCAFillModeForwards];
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
  [rotationAnimation setDuration:forwardArcRotationTime];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];

  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)setSpinDirection:(SpinDirection)newSpinDirection {
  BOOL spinReverse = (newSpinDirection == SpinDirection::REVERSE);

  if (spinReverse == spinReverse_) {
    return;
  }
  spinReverse_ = spinReverse;

  spinnerAnimation_.reset();
  rotationAnimation_.reset();
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];

  [super updateAnimation:nil];
}

@end
