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

@implementation TabSpinnerIconView

@synthesize tabLoadingState = tabLoadingState_;

- (CALayer*)makeBackingLayer {
  CALayer* parentLayer = [super makeBackingLayer];

  CGRect bounds = [self bounds];

  iconLayer_ = [CALayer layer];
  [iconLayer_ setBounds:bounds];
  [parentLayer addSublayer:iconLayer_];
  [iconLayer_ setPosition:CGPointMake(NSMidX(bounds), NSMidY(bounds))];
  [iconLayer_ setContents:icon_.get()];

  return parentLayer;
}

- (BOOL)showingAnIcon {
  return tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone;
}

- (BOOL)hasDarkTheme {
  return [[self window] respondsToSelector:@selector(hasDarkTheme)] &&
         [[self window] hasDarkTheme];
}

- (NSColor*)spinnerColor {
  if ([self showingAnIcon]) {
    return [NSColor clearColor];
  }

  if ([self hasDarkTheme]) {
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
  static CGFloat reverseArcLength =
      kDegrees90 * ([SpinnerView arcUnitRadius] * 2);

  return tabLoadingState_ == kTabWaiting ? reverseArcLength : [super arcLength];
}

+ (NSImage*)sadTabIconForDarkTheme:(BOOL)hasDarkTheme {
  static NSImage* sadTabIcon = ui::ResourceBundle::GetSharedInstance()
                                   .GetNativeImageNamed(IDR_CRASH_SAD_FAVICON)
                                   .CopyNSImage();

  if (hasDarkTheme) {
    NSRect bounds = NSZeroRect;
    bounds.size = [sadTabIcon size];

    NSImage* sadTabDarkTheme =
        [[[NSImage alloc] initWithSize:bounds.size] autorelease];
    [sadTabDarkTheme lockFocus];
    [[NSColor whiteColor] set];
    NSRectFill(bounds);
    [sadTabIcon drawInRect:bounds
                  fromRect:bounds
                 operation:NSCompositeDestinationIn
                  fraction:1.0];
    [sadTabDarkTheme unlockFocus];

    return sadTabDarkTheme;
  }

  return sadTabIcon;
}

- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)flag {
  // We don't expect any other animations to arrive here.
  CHECK([[animation valueForKey:kAnimationID]
      isEqualToString:kSadTabAnimationName]);

  // Switch to the sad tab icon.
  [iconLayer_ setContents:[TabSpinnerIconView
                              sadTabIconForDarkTheme:[self hasDarkTheme]]];

  // Animate the layer back into view.
  const CGFloat offset = [self bounds].size.height;
  [sadTabAnimation_ setFromValue:@(-offset)];
  [sadTabAnimation_ setToValue:@(0)];
  [sadTabAnimation_ setDuration:kSadTabAnimationTime];

  // Clear the delegate so that the frameworks don't call here again.
  [sadTabAnimation_ setDelegate:nil];

  // Restart the animation.
  [iconLayer_ addAnimation:sadTabAnimation_.get() forKey:kSadTabAnimationName];
}

- (void)initializeSadTabAnimation {
  CGRect bounds = [self bounds];
  const CGFloat offset = bounds.size.height;

  // Animate the icon out of view.
  CABasicAnimation* animateOutAnimation = [CABasicAnimation animation];
  animateOutAnimation.keyPath = @"transform.translation.y";
  [animateOutAnimation setFromValue:@(0)];
  [animateOutAnimation setToValue:@(-offset)];
  // If there's no icon the order out animation will complete immediately.
  CGFloat orderOutDuration = [iconLayer_ contents] ? kSadTabAnimationTime : 0;
  [animateOutAnimation setDuration:orderOutDuration];
  [animateOutAnimation setRemovedOnCompletion:NO];
  [animateOutAnimation setFillMode:kCAFillModeForwards];
  [animateOutAnimation setRepeatCount:0];

  // Find out when the animate out completes so that we can animate the sad
  // tab icon in.
  [animateOutAnimation setDelegate:self];
  [animateOutAnimation setValue:kSadTabAnimationName forKey:kAnimationID];

  sadTabAnimation_.reset([animateOutAnimation retain]);
  [iconLayer_ addAnimation:sadTabAnimation_.get() forKey:kSadTabAnimationName];
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

  if (tabLoadingState_ == kTabCrashed || tabLoadingState_ == kTabDone) {
    [shapeLayer_ setPath:nullptr];
    [iconLayer_ setContents:icon_.get()];
  } else {
    [iconLayer_ setContents:nullptr];
  }

  spinnerAnimation_.reset();
  rotationAnimation_.reset();
  sadTabAnimation_.reset();
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];
  [iconLayer_ removeAllAnimations];

  [super updateAnimation:nil];
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
