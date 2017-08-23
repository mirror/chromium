// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/toolbar/toolbar_button.h"

#import "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolbarButton ()
@property(nonatomic, strong) UIImage* normalImage;
@property(nonatomic, strong) UIImage* highlightedImage;
@end

@implementation ToolbarButton
@synthesize visibilityMask = _visibilityMask;
@synthesize hiddenInCurrentSizeClass = _hiddenInCurrentSizeClass;
@synthesize hiddenInCurrentState = _hiddenInCurrentState;
@synthesize normalImage = _normalImage;
@synthesize highlightedImage = _highlightedImage;

+ (instancetype)toolbarButtonWithImageForNormalState:(UIImage*)normalImage
                            imageForHighlightedState:(UIImage*)highlightedImage
                               imageForDisabledState:(UIImage*)disabledImage {
  ToolbarButton* button = [[self class] buttonWithType:UIButtonTypeCustom];
  [button setImage:normalImage forState:UIControlStateNormal];
  [button setImage:highlightedImage forState:UIControlStateHighlighted];
  [button setImage:disabledImage forState:UIControlStateDisabled];
  button.normalImage = normalImage;
  button.highlightedImage = highlightedImage;
  button.titleLabel.textAlignment = NSTextAlignmentCenter;
  button.translatesAutoresizingMaskIntoConstraints = NO;
  return button;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  // If the UIButton title has text it will center it on top of the image,
  // this is currently used for the TabStripButton which displays the
  // total number of tabs.
  if (self.titleLabel.text) {
    CGSize size = self.bounds.size;
    CGPoint center = CGPointMake(size.width / 2, size.height / 2);
    self.imageView.center = center;
    self.imageView.frame = AlignRectToPixel(self.imageView.frame);
    self.titleLabel.frame = self.bounds;
  }
}

#pragma mark - Public Methods

- (void)setHighlightedAppearance:(BOOL)highlighted {
  if (highlighted) {
    [self setImage:self.highlightedImage forState:UIControlStateNormal];
    [self setImage:self.normalImage forState:UIControlStateHighlighted];
  } else {
    [self setImage:self.normalImage forState:UIControlStateNormal];
    [self setImage:self.highlightedImage forState:UIControlStateHighlighted];
  }
}

- (void)updateHiddenInCurrentSizeClass {
  BOOL newHiddenValue = YES;
  switch (self.traitCollection.horizontalSizeClass) {
    case UIUserInterfaceSizeClassRegular:
      newHiddenValue =
          !(self.visibilityMask & ToolbarComponentVisibilityRegularWidth);
      break;
    case UIUserInterfaceSizeClassCompact:
      // First check if the button should be visible only when it's enabled,
      // if not, check if it should be visible in this case.
      if (self.visibilityMask &
          ToolbarComponentVisibilityCompactWidthOnlyWhenEnabled) {
        newHiddenValue = !self.enabled;
      } else if (self.visibilityMask & ToolbarComponentVisibilityCompactWidth) {
        newHiddenValue = NO;
      }
      break;
    case UIUserInterfaceSizeClassUnspecified:
    default:
      break;
  }
  if (newHiddenValue != self.hiddenInCurrentSizeClass) {
    self.hiddenInCurrentSizeClass = newHiddenValue;
    [self setHiddenForCurrentStateAndSizeClass];
  }
}

- (void)setHiddenForCurrentStateAndSizeClass {
  self.hidden = self.hiddenInCurrentState || self.hiddenInCurrentSizeClass;
}

@end
