// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_spinner_icon_view.h"

#include "base/mac/scoped_cftyperef.h"
#include "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
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
const CGFloat kWaitingStrokeAlpha = 0.5;
const CGFloat kSadTabAnimationTime = 0.5;
}  // namespace

@implementation TabSpinnerIconView : SpinnerView

@synthesize tabLoadingState = tabLoadingState_;

+ (CGFloat)reverseArcLength {
  // The arc is 90 degrees of circumference.
  static CGFloat reverseArcLength = kDegrees90 * ([self spinnerArcRadius] * 2);
  return reverseArcLength;
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

  if (hasDarkTheme) {
    return tabLoadingState_ == kTabWaiting
               ? [[NSColor whiteColor]
                     colorWithAlphaComponent:kWaitingStrokeAlpha]
               : [NSColor whiteColor];
  }

  const ui::ThemeProvider* theme = nullptr;
  if ([[self window] respondsToSelector:@selector(themeProvider)]) {
    theme = [[self window] themeProvider];
  }

  if (tabLoadingState_ == kTabWaiting) {
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

- (CGFloat)spinnerStartAngle {
  return tabLoadingState_ == kTabWaiting ? kDegrees270
                                         : [super spinnerStartAngle];
}

- (CGFloat)spinnerEndAngleDelta {
  return tabLoadingState_ == kTabWaiting ? kDegrees180
                                         : [super spinnerEndAngleDelta];
}

- (NSArray*)spinnerDashPattern {
  return tabLoadingState_ == kTabWaiting
             ? @[ @([TabSpinnerIconView reverseArcLength] *
                    [self scaleFactor]) ]
             : [super spinnerDashPattern];
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
  const CGFloat reverseArcAnimationTime =
      [SpinnerView spinnerRotationTime] / 2.0;

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
  [rotationAnimation setDuration:[TabSpinnerIconView spinnerRotationTime]];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];

  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState
                      icon:(NSImage*)icon {
  // Always update the tab done icon, otherwise exit if the state has not
  // changed.
  if (newLoadingState != kTabDone && tabLoadingState_ == newLoadingState) {
    return;
  }

  tabLoadingState_ = newLoadingState;

  if (tabLoadingState_ == kTabCrashed) {
    static NSImage* sadTabIcon = ResourceBundle::GetSharedInstance()
                                     .GetNativeImageNamed(IDR_CRASH_SAD_FAVICON)
                                     .CopyNSImage();
    icon = sadTabIcon;
  }

  if (tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone) {
    [shapeLayer_ setPath:nullptr];
    [shapeLayer_ setContents:icon];
  }

  spinnerAnimation_.reset();
  rotationAnimation_.reset();
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];

  [self updateAnimation:nil];
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState {
  [self setTabLoadingState:newLoadingState icon:nil];
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

@end
