// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bars/bookmark_context_bar.h"
#include "base/logging.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Shadow opacity for the clear browsing bar.
CGFloat kShadowOpacity = 0.2f;
// Horizontal margin for the contents of BookmarkContextBar.
CGFloat kHorizontalMargin = 8.0f;
// Enum to specify button position in the clear browsing bar.
}  // namespace

@interface BookmarkContextBar () {
}
// Button at the leading position.
@property(nonatomic, strong) UIButton* leadingButton;
// Button at the center position.
@property(nonatomic, strong) UIButton* centerButton;
// Button at the trailing position.
@property(nonatomic, strong) UIButton* trailingButton;
// Stack view for arranging the buttons.
@property(nonatomic, strong) UIStackView* stackView;

@end

@implementation BookmarkContextBar
@synthesize delegate = _delegate;
@synthesize leadingButton = _leadingButton;
@synthesize centerButton = _centerButton;
@synthesize trailingButton = _trailingButton;
@synthesize stackView = _stackView;

#pragma mark Private Methods

- (UIButton*)getButton:(ContextBarButton)contextBarButton {
  switch (contextBarButton) {
    case ContextBarLeadingButton: {
      return _leadingButton;
      break;
    }
    case ContextBarCenterButton: {
      return _centerButton;
      break;
    }
    case ContextBarTrailingButton: {
      return _trailingButton;
      break;
    }
    default: { NOTREACHED(); }
  }
}

- (void)initButtonStyle:(ContextBarButton)contextBarButton {
  UIButton* button = [self getButton:contextBarButton];
  [button setBackgroundColor:[UIColor whiteColor]];
  [button setTitleColor:[[MDCPalette bluePalette] tint500]
               forState:UIControlStateNormal];
  [[button titleLabel]
      setFont:[[MDCTypography fontLoader] regularFontOfSize:14]];
  [button setTranslatesAutoresizingMaskIntoConstraints:NO];
  button.hidden = YES;

  switch (contextBarButton) {
    case ContextBarLeadingButton: {
      button.contentHorizontalAlignment =
          UIControlContentHorizontalAlignmentLeft;
      break;
    }
    case ContextBarCenterButton: {
      button.contentHorizontalAlignment =
          UIControlContentHorizontalAlignmentCenter;
      break;
    }
    case ContextBarTrailingButton: {
      button.contentHorizontalAlignment =
          UIControlContentHorizontalAlignmentRight;
      break;
    }
    default: { NOTREACHED(); }
  }
}
- (void)setButtonTarget:(ContextBarButton)contextBarButton action:(SEL)action {
  UIButton* button = [self getButton:contextBarButton];
  [button addTarget:self
                action:action
      forControlEvents:UIControlEventTouchUpInside];
}

#pragma mark Public Methods

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _leadingButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [self initButtonStyle:ContextBarLeadingButton];
    [self setButtonTarget:ContextBarLeadingButton
                   action:@selector(leadingButtonClicked:)];

    _centerButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [self initButtonStyle:ContextBarCenterButton];
    [self setButtonTarget:ContextBarCenterButton
                   action:@selector(centerButtonClicked:)];

    _trailingButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [self initButtonStyle:ContextBarTrailingButton];
    [self setButtonTarget:ContextBarTrailingButton
                   action:@selector(trailingButtonClicked:)];

    _stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
      _leadingButton, _centerButton, _trailingButton
    ]];
    _stackView.alignment = UIStackViewAlignmentFill;
    _stackView.distribution = UIStackViewDistributionFillEqually;
    _stackView.axis = UILayoutConstraintAxisHorizontal;

    [self addSubview:_stackView];
    _stackView.translatesAutoresizingMaskIntoConstraints = NO;
    _stackView.layoutMarginsRelativeArrangement = YES;
    [NSLayoutConstraint activateConstraints:@[
      [_stackView.layoutMarginsGuide.leadingAnchor
          constraintEqualToAnchor:self.leadingAnchor
                         constant:kHorizontalMargin],
      [_stackView.layoutMarginsGuide.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor
                         constant:-kHorizontalMargin],
      [_stackView.topAnchor constraintEqualToAnchor:self.topAnchor],
      [_stackView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    ]];

    [self setBackgroundColor:[UIColor whiteColor]];
    [[self layer] setShadowOpacity:kShadowOpacity];
  }
  return self;
}

- (void)setButtonTitle:(NSString*)title forButton:(ContextBarButton)button {
  [[self getButton:button] setTitle:title forState:UIControlStateNormal];
  [[self getButton:button] setTitle:title forState:UIControlStateNormal];
}

- (void)setButtonStyle:(ContextBarButtonStyle)style
             forButton:(ContextBarButton)button {
  UIColor* textColor = style == ContextBarButtonStyleDelete
                           ? [[MDCPalette redPalette] tint500]
                           : [[MDCPalette bluePalette] tint500];
  [[self getButton:button] setTitleColor:textColor
                                forState:UIControlStateNormal];
}

- (void)setButtonVisibilityButton:(BOOL)visible
                        forButton:(ContextBarButton)button {
  [self getButton:button].hidden = !visible;
}

- (void)leadingButtonClicked:(UIButton*)button {
  if (!self.delegate) {
    NOTREACHED();
  }
  [self.delegate leadingButtonClicked];
}

- (void)centerButtonClicked:(UIButton*)button {
  if (!self.delegate) {
    NOTREACHED();
  }
  [self.delegate leadingButtonClicked];
}

- (void)trailingButtonClicked:(UIButton*)button {
  if (!self.delegate) {
    NOTREACHED();
  }
  [self.delegate trailingButtonClicked];
}

@end
