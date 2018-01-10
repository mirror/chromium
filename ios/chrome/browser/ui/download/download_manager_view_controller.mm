// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/download_manager_view_controller.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// A margin for every control alement (status label, action button, close
// button). Defines the minimal distance between elements.
const CGFloat kElementMargin = 16;

// Returns formatted size string.
NSString* GetSizeString(long long size_in_bytes) {
  return [NSByteCountFormatter
      stringFromByteCount:size_in_bytes
               countStyle:NSByteCountFormatterCountStyleFile];
}
}  // namespace

@interface DownloadManagerViewController () {
  // Shadow view sits on top of background view. Union of shadow and background
  // views fills self.view area.
  UIImageView* _shadow;
  UIView* _background;
}
@end

@implementation DownloadManagerViewController

@synthesize delegate = _delegate;
@synthesize fileName = _fileName;
@synthesize countOfBytesReceived = _countOfBytesReceived;
@synthesize countOfBytesExpectedToReceive = _countOfBytesExpectedToReceive;
@synthesize state = _state;
@synthesize closeButton = _closeButton;
@synthesize statusLabel = _statusLabel;
@synthesize actionButton = _actionButton;

#pragma mark - UIViewController overrides

- (void)viewDidLoad {
  [super viewDidLoad];

  [self.view addSubview:[self shadow]];
  [self.view addSubview:[self background]];
  [self.view addSubview:[self closeButton]];
  [self.view addSubview:[self statusLabel]];
  [self.view addSubview:[self actionButton]];
}

- (void)updateViewConstraints {
  [super updateViewConstraints];

  UIView* view = self.view;
  view.translatesAutoresizingMaskIntoConstraints = NO;
  UIView* superview = view.superview;
  [NSLayoutConstraint activateConstraints:@[
    [view.heightAnchor constraintEqualToAnchor:[self statusLabel].heightAnchor constant:kElementMargin * 2],
    [view.leftAnchor constraintEqualToAnchor:superview.leftAnchor],
    [view.rightAnchor constraintEqualToAnchor:superview.rightAnchor],
  ]];

  [NSLayoutConstraint activateConstraints:@[
    [[self shadow].leftAnchor constraintEqualToAnchor:view.leftAnchor],
    [[self shadow].rightAnchor constraintEqualToAnchor:view.rightAnchor],
    [[self shadow].topAnchor constraintEqualToAnchor:view.topAnchor],
    [[self shadow].heightAnchor constraintEqualToConstant:[self shadow].image.size.height],
  ]];
  [NSLayoutConstraint activateConstraints:@[
    [[self background].leftAnchor constraintEqualToAnchor:view.leftAnchor],
    [[self background].rightAnchor constraintEqualToAnchor:view.rightAnchor],
    [[self background].bottomAnchor constraintEqualToAnchor:view.bottomAnchor],
    [[self background].topAnchor constraintEqualToAnchor:[self shadow].bottomAnchor],
  ]];

  [self updateCloseButtonConstraints];
  [self updateStatusLabelConstraints];
  [self updateActionButtonConstraints];
}

#pragma mark - Public Properties

- (void)setFileName:(NSString*)fileName {
  if (![_fileName isEqual:fileName]) {
    _fileName = [fileName copy];
    [self updateStatusLabel];
  }
}

- (void)setCountOfBytesReceived:(int64_t)value {
  if (_countOfBytesReceived != value) {
    _countOfBytesReceived = value;
    [self updateStatusLabel];
  }
}

- (void)setCountOfBytesExpectedToReceive:(int64_t)value {
  if (_countOfBytesExpectedToReceive != value) {
    _countOfBytesExpectedToReceive = value;
    [self updateStatusLabel];
  }
}

- (void)setState:(DownloadManagerState)state {
  if (_state != state) {
    _state = state;
    [self updateStatusLabel];
    [self updateActionButton];
  }
}

#pragma mark - UI elements

- (UIImageView*)shadow {
  if (!_shadow) {
    UIImage* shadowImage = [UIImage imageNamed:@"infobar_shadow"];
    _shadow = [[UIImageView alloc] initWithImage:shadowImage];
    _shadow.translatesAutoresizingMaskIntoConstraints = NO;
  }
  return _shadow;
}

- (UIView*)background {
  if (!_background) {
    _background = [[UIView alloc] initWithFrame:CGRectZero];
    _background.translatesAutoresizingMaskIntoConstraints = NO;
    _background.backgroundColor = [UIColor whiteColor];
  }
  return _background;
}

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

- (UIButton*)statusLabel {
  if (!_statusLabel) {
    _statusLabel = [[UIButton alloc] initWithFrame:CGRectZero];
    _statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _statusLabel.backgroundColor = [UIColor clearColor];
    _statusLabel.titleLabel.font = [MDCTypography subheadFont];
    [self updateStatusLabel];
  }
  return _statusLabel;
}

- (UIButton*)actionButton {
  if (!_actionButton) {
    _actionButton = [UIButton buttonWithType:UIButtonTypeCustom];
    _actionButton.translatesAutoresizingMaskIntoConstraints = NO;
    _actionButton.exclusiveTouch = YES;
    [_actionButton setTitleColor:[MDCPalette bluePalette].tint500
                 forState:UIControlStateNormal];

    [_actionButton addTarget:self
                  action:@selector(didTapActionButton)
        forControlEvents:UIControlEventTouchUpInside];
    [self updateActionButton];
  }
  return _actionButton;
}

#pragma mark - Constraints

- (void)updateCloseButtonConstraints {
  UIView* view = self.view;
  UIButton* button = [self closeButton];
  CGSize imageSize = button.imageView.image.size;
  CGFloat buttonWidth = imageSize.width + kElementMargin * 2;
  CGFloat buttonHeight = imageSize.height + kElementMargin * 2;
  [NSLayoutConstraint activateConstraints:@[
    [button.widthAnchor constraintEqualToConstant:buttonWidth],
    [button.heightAnchor constraintEqualToConstant:buttonHeight],
    [button.centerYAnchor constraintEqualToAnchor:view.centerYAnchor],
    [button.trailingAnchor constraintEqualToAnchor:view.trailingAnchor],
  ]];
}

- (void)updateStatusLabelConstraints {
  [NSLayoutConstraint activateConstraints:@[
    [[self statusLabel].centerYAnchor
        constraintEqualToAnchor:self.view.centerYAnchor],
    [[self statusLabel].leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor
                       constant:kElementMargin],
    [[self statusLabel].trailingAnchor
        constraintLessThanOrEqualToAnchor:[self actionButton].leadingAnchor
                                 constant:-kElementMargin],
  ]];
}

- (void)updateActionButtonConstraints {
  UIButton* button = [self actionButton];
  [NSLayoutConstraint activateConstraints:@[
    [button.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    [button.trailingAnchor
        constraintEqualToAnchor:[self closeButton].leadingAnchor],
  ]];
}

#pragma mark - Actions

- (void)didTapCloseButton {
  SEL selector = @selector(downloadManagerViewControllerDidClose:);
  if ([_delegate respondsToSelector:selector]) {
    [_delegate downloadManagerViewControllerDidClose:self];
  }
}

- (void)didTapActionButton {
  switch (self.state) {
    case kDownloadManagerStateNotStarted: {
      SEL selector = @selector(downloadManagerViewControllerDidStartDownload:);
      if ([_delegate respondsToSelector:selector]) {
        [_delegate downloadManagerViewControllerDidStartDownload:self];
      }
      break;
    }
    case kDownloadManagerStateInProgress: {
      // The button should not be visible.
      NOTREACHED();
      break;
    }
    case kDownloadManagerStateSuceeded: {
      SEL selector =
          @selector(downloadManagerViewController:presentOpenInMenuFromRect:);
      if ([_delegate respondsToSelector:selector]) {
        [_delegate downloadManagerViewController:self
                       presentOpenInMenuFromRect:[self actionButton].frame];
      }
      break;
    }
    case kDownloadManagerStateFailed: {
      SEL selector = @selector(downloadManagerViewControllerDidStartDownload:);
      if ([_delegate respondsToSelector:selector]) {
        [_delegate downloadManagerViewControllerDidStartDownload:self];
      }
      break;
    }
  }
}

#pragma mark - UI Updates

// Updates status name label text depending on |state|.
- (void)updateStatusLabel {
  NSString* statusText = nil;
  switch (self.state) {
    case kDownloadManagerStateNotStarted:
      statusText = [NSString stringWithFormat:@"%@ - %@", self.fileName, GetSizeString(self.countOfBytesExpectedToReceive)];
      break;
    case kDownloadManagerStateInProgress:
      statusText = [NSString stringWithFormat:@"Downloading... %@/%@", GetSizeString(self.countOfBytesReceived),
          GetSizeString(self.countOfBytesExpectedToReceive)];
      break;
    case kDownloadManagerStateSuceeded:
      statusText = self.fileName;
      break;
    case kDownloadManagerStateFailed:
      statusText = @"Couldn't Download";
      break;
  }

  [[self statusLabel] setTitle:statusText forState:UIControlStateNormal];
  [[self statusLabel] setTitleColor:[UIColor blackColor]
                         forState:UIControlStateNormal];
  [[self statusLabel] sizeToFit];
}

// Updates title and hidden state for action button depending on |state|.
- (void)updateActionButton {
  NSString* title = nil;
  switch (self.state) {
    case kDownloadManagerStateNotStarted:
      title = @"Download";
      break;
    case kDownloadManagerStateInProgress:
      break;
    case kDownloadManagerStateSuceeded:
      title = @"Open In...";
      break;
    case kDownloadManagerStateFailed:
      title = @"Try Again";
      break;
  }

  [[self actionButton] setTitle:title forState:UIControlStateNormal];
  [self actionButton].hidden = self.state == kDownloadManagerStateInProgress;
}

@end
