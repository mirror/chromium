// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/download_manager_view_controller.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kViewHeight = 60;

// Copied from infobar_view.mm
const CGFloat kCloseButtonInnerPadding = 16;
const CGFloat kLabelLineSpacing = 5;
}

@implementation DownloadManagerViewController
@synthesize delegate = _delegate;
@synthesize fileName = _fileName;
@synthesize closeButton = _closeButton;
@synthesize statusLabel = _statusLabel;

#pragma mark - UIViewController overrides

- (void)viewDidLoad {
  [super viewDidLoad];

  self.view.backgroundColor = [UIColor whiteColor];

  self.view.layer.shadowOffset = CGSizeMake(0, -3);
  self.view.layer.shadowRadius = 3;
  self.view.layer.shadowOpacity = 0.3;

  [self.view addSubview:self.closeButton];
  [self.view addSubview:self.statusLabel];
}

- (void)updateViewConstraints {
  [super updateViewConstraints];

  UIView* view = self.view;
  view.translatesAutoresizingMaskIntoConstraints = NO;
  [NSLayoutConstraint activateConstraints:@[
    [view.heightAnchor constraintEqualToConstant:kViewHeight],
    [view.leftAnchor constraintEqualToAnchor:view.superview.leftAnchor],
    [view.rightAnchor constraintEqualToAnchor:view.superview.rightAnchor],
  ]];

  [self updateCloseButtonConstraints];
  [self updateDownloadLabelConstraints];
}

#pragma mark - UI elements

- (UIButton*)closeButton {
  if (!_closeButton) {
    _closeButton = [UIButton buttonWithType:UIButtonTypeCustom];
    _closeButton.translatesAutoresizingMaskIntoConstraints = NO;
    _closeButton.exclusiveTouch = YES;
    _closeButton.accessibilityLabel = l10n_util::GetNSString(IDS_CLOSE);

    // TODO(crbug/228611): Add IDR_ constant and use GetNativeImageNamed().
    UIImage* image = [UIImage imageNamed:@"infobar_close"];
    [_closeButton setImage:image forState:UIControlStateNormal];

    [_closeButton addTarget:self
                     action:@selector(didTapCloseButton)
           forControlEvents:UIControlEventTouchUpInside];
  }
  return _closeButton;
}

- (UILabel*)statusLabel {
  if (!_statusLabel) {
    _statusLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _statusLabel.backgroundColor = [UIColor clearColor];
    _statusLabel.numberOfLines = 0;
    [self updateStatusLabel];
  }
  return _statusLabel;
}

#pragma mark - Constraints

- (void)updateCloseButtonConstraints {
  UIView* view = self.view;
  UIButton* button = self.closeButton;
  CGSize imageSize = button.imageView.image.size;
  CGFloat buttonWidth = imageSize.width + kCloseButtonInnerPadding * 2;
  CGFloat buttonHeight = imageSize.height + kCloseButtonInnerPadding * 2;
  [NSLayoutConstraint activateConstraints:@[
    [button.widthAnchor constraintEqualToConstant:buttonWidth],
    [button.heightAnchor constraintEqualToConstant:buttonHeight],
    [button.centerYAnchor constraintEqualToAnchor:view.centerYAnchor],
    [button.trailingAnchor constraintEqualToAnchor:view.trailingAnchor],
  ]];
}

- (void)updateDownloadLabelConstraints {
  UILabel* label = self.statusLabel;
  [NSLayoutConstraint activateConstraints:@[
    [label.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    [label.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor
                                        constant:16],
  ]];
}

#pragma mark - Actions

- (void)didTapCloseButton {
  SEL selector = @selector(downloadManagerViewControllerDidClose:);
  if ([_delegate respondsToSelector:selector]) {
    [_delegate downloadManagerViewControllerDidClose:self];
  }
}

#pragma mark - UI Updates

- (NSString*)statusLabelText {
  return self.fileName;
}

- (NSMutableDictionary*)statusLabelAttributes {
  NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
  style.lineBreakMode = NSLineBreakByWordWrapping;
  style.lineSpacing = kLabelLineSpacing;
  NSMutableDictionary* attributes =
      [NSMutableDictionary dictionaryWithDictionary:@{
        NSParagraphStyleAttributeName : style,
        NSFontAttributeName : [MDCTypography subheadFont],
      }];
  // TODO: which color should this be?
  attributes[NSForegroundColorAttributeName] = [MDCPalette bluePalette].tint500;

  return attributes;
}

// Updates status label text and color depending on |state|.
- (void)updateStatusLabel {
  self.statusLabel.attributedText =
      [[NSAttributedString alloc] initWithString:[self statusLabelText]
                                      attributes:[self statusLabelAttributes]];
  [self.statusLabel sizeToFit];
}

@end
