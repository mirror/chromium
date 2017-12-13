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
constexpr CGFloat kDegrees90 = gfx::DegToRad(90.0f);
constexpr CGFloat kDegrees180 = gfx::DegToRad(180.0f);
constexpr CGFloat kDegrees270 = gfx::DegToRad(270.0f);
constexpr CGFloat kDegrees360 = gfx::DegToRad(360.0f);
constexpr CGFloat kWaitingStrokeAlpha = 0.5;
constexpr CGFloat kSadTabAnimationTime = 0.5;
}  // namespace

@interface TabSpinnerIconView () {
  base::scoped_nsobject<NSImage> icon_;
  base::scoped_nsobject<CABasicAnimation> sadTabAnimation_;
  CALayer* iconLayer_;  // Weak.
}
@end

@implementation TabSpinnerIconView

@synthesize tabLoadingState = tabLoadingState_;

- (CALayer*)makeBackingLayer {
  CALayer* parentLayer = [super makeBackingLayer];

  CGRect bounds = [self bounds];

  iconLayer_ = [CALayer layer];
  [iconLayer_ setBounds:bounds];
  [parentLayer addSublayer:iconLayer_];
  [iconLayer_ setPosition:CGPointMake(NSMidX(bounds), NSMidY(bounds))];
  CGFloat recommendedLayerContentsScale =
      [icon_ recommendedLayerContentsScale:0.0];
  [iconLayer_ setContents:[icon_ layerContentsForContentsScale:
                                     recommendedLayerContentsScale]];

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

- (void)initializeSadTabAnimation {
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = [iconLayer_ contents] ? kSadTabAnimationTime : 0;

    const CGFloat offset = self.bounds.size.height;
    [iconLayer_ setValue:@(-offset) forKeyPath:@"transform.translation.y"];
  }
      completionHandler:^{
        iconLayer_.contents =
            [[self class] sadTabIconForDarkTheme:[self hasDarkTheme]];
        [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
          context.duration = kSadTabAnimationTime;

          [iconLayer_ setValue:@0 forKeyPath:@"transform.translation.y"];
        }
                            completionHandler:nil];
      }];
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
