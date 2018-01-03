// Copyright 2017 The Chromium Authors. All rights reserved.
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
const CGFloat kViewHeight = 60;

// Copied from infobar_view.mm
const CGFloat kCloseButtonInnerPadding = 16;

// Returns formatted size string.
NSString* GetSizeString(long long size_in_bytes) {
  return [NSByteCountFormatter
      stringFromByteCount:size_in_bytes
               countStyle:NSByteCountFormatterCountStyleFile];
}
}

@implementation DownloadManagerViewController
@synthesize delegate = _delegate;
@synthesize fileName = _fileName;
@synthesize expectedFileSize = _expectedFileSize;
@synthesize state = _state;
@synthesize closeButton = _closeButton;
@synthesize fileNameLabel = _fileNameLabel;
@synthesize statusLabel = _statusLabel;
@synthesize progressLabel = _progressLabel;
@synthesize openInButton = _openInButton;
@synthesize restartButton = _restartButton;

#pragma mark - UIViewController overrides

- (void)viewDidLoad {
  [super viewDidLoad];

  self.view.backgroundColor = [UIColor whiteColor];

  self.view.layer.shadowOffset = CGSizeMake(0, -3);
  self.view.layer.shadowRadius = 3;
  self.view.layer.shadowOpacity = 0.3;

  [self.view addSubview:self.closeButton];
  [self.view addSubview:self.fileNameLabel];
  [self.view addSubview:self.statusLabel];
  [self.view addSubview:self.progressLabel];
  [self.view addSubview:self.openInButton];
  [self.view addSubview:self.restartButton];
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
  [self updateLabelsConstraints];
  [self updateOpenInButtonConstraints];
  [self updateRestartButtonConstraints];
}

#pragma mark - Public Properties

- (void)setFileName:(NSString*)fileName {
  if (![_fileName isEqual:fileName]) {
    _fileName = [fileName copy];
    [self updateFileNameLabel];
  }
}

- (void)setExpectedFileSize:(long long)expectedFileSize {
  if (_expectedFileSize != expectedFileSize) {
    _expectedFileSize = expectedFileSize;
    [self updateProgressLabel];
  }
}

- (void)setState:(DownloadManagerState)state {
  if (_state != state) {
    _state = state;
    [self updateFileNameLabel];
    [self updateStatusLabel];
    [self updateProgressLabel];
    [self updateOpenInButton];
    [self updateRestartButton];
  }
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
                     action:@selector(didClose)
           forControlEvents:UIControlEventTouchUpInside];
  }
  return _closeButton;
}

- (UIButton*)fileNameLabel {
  if (!_fileNameLabel) {
    _fileNameLabel = [self newLabel];
    [self updateFileNameLabel];
  }
  return _fileNameLabel;
}

- (UIButton*)statusLabel {
  if (!_statusLabel) {
    _statusLabel = [self newLabel];
    [self updateStatusLabel];
  }
  return _statusLabel;
}

- (UIButton*)progressLabel {
  if (!_progressLabel) {
    _progressLabel = [self newLabel];
    [self updateProgressLabel];
  }
  return _progressLabel;
}

- (UIButton*)openInButton {
  if (!_openInButton) {
    _openInButton =
        [self newButtonWithTitle:l10n_util::GetNSString(IDS_IOS_OPEN_IN)
                          action:@selector(didRequestOpen)];
    [self updateOpenInButton];
  }
  return _openInButton;
}

- (UIButton*)restartButton {
  if (!_restartButton) {
    _restartButton = [self newButtonWithTitle:@"Restart"
                                       action:@selector(didRequestRestart)];
    [self updateRestartButton];
  }
  return _restartButton;
}

- (UIButton*)newLabel {
  UIButton* result = [[UIButton alloc] initWithFrame:CGRectZero];
  result.translatesAutoresizingMaskIntoConstraints = NO;
  result.backgroundColor = [UIColor clearColor];
  [result addTarget:self
                action:@selector(didStart)
      forControlEvents:UIControlEventTouchUpInside];
  result.titleLabel.font = [MDCTypography subheadFont];
  return result;
}

- (UIButton*)newButtonWithTitle:(NSString*)title action:(SEL)action {
  UIButton* result = [UIButton buttonWithType:UIButtonTypeCustom];
  result.translatesAutoresizingMaskIntoConstraints = NO;
  result.exclusiveTouch = YES;
  result.accessibilityLabel = title;
  [result setTitle:title forState:UIControlStateNormal];
  [result setTitleColor:[MDCPalette bluePalette].tint500
               forState:UIControlStateNormal];

  [result addTarget:self
                action:action
      forControlEvents:UIControlEventTouchUpInside];
  [result sizeToFit];
  return result;
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

- (void)updateOpenInButtonConstraints {
  UIButton* button = self.openInButton;
  [NSLayoutConstraint activateConstraints:@[
    [button.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    [button.trailingAnchor
        constraintEqualToAnchor:self.closeButton.leadingAnchor],
  ]];
}

- (void)updateRestartButtonConstraints {
  UIButton* button = self.restartButton;
  [NSLayoutConstraint activateConstraints:@[
    [button.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    [button.trailingAnchor
        constraintEqualToAnchor:self.closeButton.leadingAnchor],
  ]];
}

- (void)updateLabelsConstraints {
  [NSLayoutConstraint activateConstraints:@[
    // Status label, width not flexible, located before file name label.
    [self.statusLabel.centerYAnchor
        constraintEqualToAnchor:self.view.centerYAnchor],
    [self.statusLabel.leadingAnchor
        constraintEqualToAnchor:self.view.leadingAnchor
                       constant:16],

    // File name label, width flexible, height is not flexible, located between
    // status and file size labels.
    [self.fileNameLabel.centerYAnchor
        constraintEqualToAnchor:self.view.centerYAnchor],
    [self.fileNameLabel.leadingAnchor
        constraintEqualToAnchor:self.statusLabel.trailingAnchor],
    [self.fileNameLabel.trailingAnchor
        constraintEqualToAnchor:self.progressLabel.leadingAnchor],

    // File size label, width not flexible, located after file name label and
    // before close button.
    [self.progressLabel.centerYAnchor
        constraintEqualToAnchor:self.view.centerYAnchor],
    [self.progressLabel.leadingAnchor
        constraintEqualToAnchor:self.fileNameLabel.trailingAnchor],
    [self.progressLabel.trailingAnchor
        constraintLessThanOrEqualToAnchor:self.closeButton.leadingAnchor],
  ]];
}

#pragma mark - Actions

- (void)didClose {
  SEL selector = @selector(downloadManagerViewControllerDidClose:);
  if ([_delegate respondsToSelector:selector]) {
    [_delegate downloadManagerViewControllerDidClose:self];
  }
}

- (void)didStart {
  if (self.state != kDownloadManagerStateNotStarted)
    return;

  SEL selector = @selector(downloadManagerViewControllerDidStartDownload:);
  if ([_delegate respondsToSelector:selector]) {
    [_delegate downloadManagerViewControllerDidStartDownload:self];
  }
}

- (void)didRequestOpen {
  SEL selector =
      @selector(downloadManagerViewController:presentOpenInMenuFromRect:);
  if ([_delegate respondsToSelector:selector]) {
    [_delegate downloadManagerViewController:self
                   presentOpenInMenuFromRect:self.openInButton.frame];
  }
}

- (void)didRequestRestart {
  DCHECK_EQ(self.state, kDownloadManagerStateFailed);
  SEL selector = @selector(downloadManagerViewControllerDidStartDownload:);
  if ([_delegate respondsToSelector:selector]) {
    [_delegate downloadManagerViewControllerDidStartDownload:self];
  }
}

#pragma mark - UI Updates

- (UIColor*)labelButtonColor {
  switch (self.state) {
    case kDownloadManagerStateNotStarted:
      // TODO: which color should this be?
      return [MDCPalette bluePalette].tint500;
    case kDownloadManagerStateInProgress:
    case kDownloadManagerStateSuceeded:
    case kDownloadManagerStateFailed:
      return [UIColor blackColor];
  }
}

// Updates file name label text and color depending on |state|.
- (void)updateFileNameLabel {
  NSString* fileNameText = nil;
  switch (self.state) {
    case kDownloadManagerStateNotStarted:
    case kDownloadManagerStateSuceeded:
      fileNameText = self.fileName;
      break;
    case kDownloadManagerStateInProgress:
    case kDownloadManagerStateFailed:
      fileNameText = @"";
      break;
  }

  [self.fileNameLabel setTitle:fileNameText forState:UIControlStateNormal];
  [self.fileNameLabel setTitleColor:[self labelButtonColor]
                           forState:UIControlStateNormal];
  [self.fileNameLabel sizeToFit];
}

// Updates status name label text and color depending on |state|.
- (void)updateStatusLabel {
  NSString* statusText = nil;
  switch (self.state) {
    case kDownloadManagerStateNotStarted:
      statusText = @"Download \"";
      break;
    case kDownloadManagerStateInProgress:
      statusText = @"Downloading...";
      break;
    case kDownloadManagerStateSuceeded:
      statusText = @"";
      break;
    case kDownloadManagerStateFailed:
      statusText = @"Couldn't Download";
      break;
  }

  [self.statusLabel setTitle:statusText forState:UIControlStateNormal];
  [self.statusLabel setTitleColor:[self labelButtonColor]
                         forState:UIControlStateNormal];
  [self.statusLabel sizeToFit];
}

// Updates file size label text and color depending on |state|.
- (void)updateProgressLabel {
  NSString* sizeText = nil;
  switch (self.state) {
    case kDownloadManagerStateNotStarted:
      // TODO: handle unknown file size.
      sizeText = [NSString
          stringWithFormat:@"\" - %@", GetSizeString(self.expectedFileSize)];
      break;
    case kDownloadManagerStateInProgress:
      // TODO: handle unknown file size.
      sizeText = [NSString
          stringWithFormat:@"/%@", GetSizeString(self.expectedFileSize)];
      break;
    case kDownloadManagerStateSuceeded:
      sizeText = @" Downloaded";
      break;
    case kDownloadManagerStateFailed:
      sizeText = @"";
      break;
  }

  [self.progressLabel setTitle:sizeText forState:UIControlStateNormal];
  [self.progressLabel setTitleColor:[self labelButtonColor]
                           forState:UIControlStateNormal];
  [self.progressLabel sizeToFit];
}

// Updates hidden state of "Open In..." button.
- (void)updateOpenInButton {
  self.openInButton.hidden = self.state != kDownloadManagerStateSuceeded;
}

// Updates hidden state of Restart button.
- (void)updateRestartButton {
  self.restartButton.hidden = self.state != kDownloadManagerStateFailed;
}

@end
