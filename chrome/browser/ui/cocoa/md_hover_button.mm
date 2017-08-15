// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <QuartzCore/QuartzCore.h>

#import "chrome/browser/ui/cocoa/md_hover_button.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#import "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {
constexpr int kDefaultIconSize = 16;
}  // namespace

@implementation MDHoverButton {
  const gfx::VectorIcon* icon_;
}
@synthesize icon = icon_;
@synthesize iconSize = iconSize_;
@synthesize hoverSuppressed = hoverSuppressed_;

+ (NSColor*)hoveringColorForDarkTheme:(BOOL)darkTheme {
  return [NSColor colorWithWhite:darkTheme ? 1 : 0 alpha:0.08];
}

+ (NSColor*)activeColorForDarkTheme:(BOOL)darkTheme {
  return [NSColor colorWithWhite:darkTheme ? 1 : 0 alpha:0.12];
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    iconSize_ = kDefaultIconSize;
    self.bezelStyle = NSRoundedBezelStyle;
    self.bordered = NO;
    self.wantsLayer = YES;
    self.layer.cornerRadius = 2;
  }
  return self;
}

- (void)setIcon:(const gfx::VectorIcon*)icon {
  icon_ = icon;
  self.needsDisplay = YES;
}

- (void)setIconSize:(int)iconSize {
  iconSize_ = iconSize;
  self.needsDisplay = YES;
}

- (void)setHoverSuppressed:(BOOL)hoverSuppressed {
  hoverSuppressed_ = hoverSuppressed;
  [self updateHoverButtonAppearanceAnimated:YES];
}

- (SkColor)iconColor {
  const ui::ThemeProvider* provider = [[self window] themeProvider];
  return [[self window] hasDarkTheme]
             ? SK_ColorWHITE
             : provider ? provider->ShouldIncreaseContrast()
                              ? SK_ColorBLACK
                              : SkColorSetRGB(0x5A, 0x5A, 0x5A)
                        : gfx::kPlaceholderColor;
}

- (void)updateHoverButtonAppearanceAnimated:(BOOL)animated {
  const BOOL darkTheme = [[self window] hasDarkTheme];
  const CGColorRef targetBackgroundColor = ^CGColorRef {
    if (hoverSuppressed_)
      return nil;
    switch (self.hoverState) {
      case kHoverStateMouseDown:
        return [self.class activeColorForDarkTheme:darkTheme].CGColor;
      case kHoverStateMouseOver:
        return [self.class hoveringColorForDarkTheme:darkTheme].CGColor;
      case kHoverStateNone:
        return nil;
    }
  }();
  if (CGColorEqualToColor(targetBackgroundColor, self.layer.backgroundColor)) {
    return;
  }
  if (!animated) {
    [self.layer removeAnimationForKey:@"hoverButtonAppearance"];
    self.layer.backgroundColor = targetBackgroundColor;
    return;
  }
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 0.25;
    CABasicAnimation* animation =
        [CABasicAnimation animationWithKeyPath:@"backgroundColor"];
    self.layer.backgroundColor = targetBackgroundColor;
    [self.layer addAnimation:animation forKey:@"hoverButtonAppearance"];
  }
                      completionHandler:nil];
}

// HoverButton overrides.

- (void)setHoverState:(HoverState)state {
  if (state == hoverState_)
    return;
  const BOOL animated =
      state != kHoverStateMouseDown && hoverState_ != kHoverStateMouseDown;
  hoverState_ = state;
  [self updateHoverButtonAppearanceAnimated:animated];
}

// NSView overrides.

- (void)viewWillDraw {
  if (!icon_ || icon_->is_empty() || iconSize_ == 0) {
    self.image = nil;
    return;
  }
  const SkColor iconColor =
      color_utils::DeriveDefaultIconColor([self iconColor]);
  self.image =
      NSImageFromImageSkia(gfx::CreateVectorIcon(*icon_, iconSize_, iconColor));
  [super viewWillDraw];
}

- (void)drawFocusRingMask {
  CGFloat radius = self.layer.cornerRadius;
  [[NSBezierPath bezierPathWithRoundedRect:self.bounds
                                   xRadius:radius
                                   yRadius:radius] fill];
}

@end
