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
NSString* const kSadTabAnimationName = @"SadTabAnimationName";
NSString* const kAnimationID = @"AnimationID";
}  // namespace

@implementation TabSpinnerIconView : SpinnerView

@synthesize tabLoadingState = tabLoadingState_;

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

- (CGFloat)arcStartAngle {
  return tabLoadingState_ == kTabWaiting ? kDegrees270 : [super arcStartAngle];
}

- (CGFloat)arcEndAngleDelta {
  return tabLoadingState_ == kTabWaiting ? kDegrees180
                                         : [super arcEndAngleDelta];
}

- (CGFloat)arcLength {
  // The reverse arc spans 90 degrees of circumference.
  static CGFloat reverseArcLength = kDegrees90 * ([SpinnerView arcRadius] * 2);

  return tabLoadingState_ == kTabWaiting ? reverseArcLength : [super arcLength];
}

- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)flag {
  CHECK([[animation valueForKey:kAnimationID]
      isEqualToString:kSadTabAnimationName]);

  static NSImage* sadTabIcon = ResourceBundle::GetSharedInstance()
                                   .GetNativeImageNamed(IDR_CRASH_SAD_FAVICON)
                                   .CopyNSImage();

  // Switch to the sad tab icon.
  [sadTabLayer_ setContents:sadTabIcon];

  // Animate back into view.
  const CGFloat offset = [self bounds].size.height;
  [sadTabAnimation_ setFromValue:@(-offset)];
  [sadTabAnimation_ setToValue:@(0)];
  [sadTabAnimation_ setDuration:kSadTabAnimationTime];

  // Clear the delegate so that we don't wind up here again.
  [sadTabAnimation_ setDelegate:nil];

  // Restart the animation.
  [sadTabLayer_ addAnimation:sadTabAnimation_.get()
                      forKey:kSadTabAnimationName];
}

- (void)initializeSadTabAnimation {
  // Don't create a new animation if there's one already.
  if (sadTabLayer_) {
    return;
  }

  CGRect bounds = [self bounds];
  const CGFloat offset = bounds.size.height;

  // Clear the icon from the shape layer.
  [shapeLayer_ setContents:nullptr];

  // Place the icon in a new layer.
  sadTabLayer_ = [CALayer layer];
  [sadTabLayer_ setBounds:bounds];
  [[self layer] addSublayer:sadTabLayer_];
  [sadTabLayer_ setPosition:CGPointMake(NSMidX(bounds), NSMidY(bounds))];
  [sadTabLayer_ setContents:icon_.get()];

  // Animate the icon out of view.
  CABasicAnimation* animateOutAnimation = [CABasicAnimation animation];
  animateOutAnimation.keyPath = @"transform.translation.y";
  [animateOutAnimation setFromValue:@(0)];
  [animateOutAnimation setToValue:@(-offset)];
  [animateOutAnimation setDuration:kSadTabAnimationTime];
  [animateOutAnimation setRemovedOnCompletion:NO];
  [animateOutAnimation setFillMode:kCAFillModeForwards];
  [animateOutAnimation setRepeatCount:0];

  // Find out when the animate out completes so that we can animate the sad
  // tab icon in.
  [animateOutAnimation setDelegate:self];
  [animateOutAnimation setValue:kSadTabAnimationName forKey:kAnimationID];

  sadTabAnimation_.reset([animateOutAnimation retain]);
  [sadTabLayer_ addAnimation:sadTabAnimation_.get()
                      forKey:kSadTabAnimationName];
}

- (void)initializeAnimation {
  if (tabLoadingState_ == kTabLoading) {
    return [super initializeAnimation];
  }
  if (tabLoadingState_ == kTabCrashed) {
    return [self initializeSadTabAnimation];
  }
  if (tabLoadingState_ != kTabWaiting) {
    return;
  }

  // Make sure |shapeLayer_|'s content scale factor matches the window's
  // backing depth (e.g. it's 2.0 on Retina Macs). Don't worry about adjusting
  // any other layers because |shapeLayer_| is the only one displaying content.
  CGFloat backingScaleFactor = [[self window] backingScaleFactor];
  [shapeLayer_ setContentsScale:backingScaleFactor];
  const CGFloat forwardArcRotationTime = [SpinnerView arcRotationTime];
  const CGFloat reverseArcAnimationTime = forwardArcRotationTime / 2.0;

  // Create the arc animation, where it grows from a short block to its full
  // length.
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
  [rotationAnimation setDuration:forwardArcRotationTime];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];

  rotationAnimation_.reset([rotationAnimation retain]);
}

- (void)setTabLoadingState:(TabLoadingState)newLoadingState {
  // Always update the tab done icon, otherwise exit if the state has not
  // changed.
  if (newLoadingState != kTabDone && tabLoadingState_ == newLoadingState) {
    return;
  }

  tabLoadingState_ = newLoadingState;

  [sadTabLayer_ removeFromSuperlayer];
  sadTabLayer_ = nil;

  if (tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone) {
    [shapeLayer_ setPath:nullptr];
    [shapeLayer_ setContents:icon_.get()];
  }

  spinnerAnimation_.reset();
  rotationAnimation_.reset();
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];

  [self updateAnimation:nil];
}

- (void)setTabDoneIcon:(NSImage*)anImage {
  icon_.reset([anImage retain]);
  [self setTabLoadingState:kTabDone];
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
