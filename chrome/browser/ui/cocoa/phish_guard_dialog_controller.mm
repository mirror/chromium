// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/phish_guard_dialog_controller.h"

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#import "ui/base/cocoa/controls/button_utils.h"
#import "ui/base/cocoa/controls/textfield_utils.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

@interface PhishGuardDialogView : NSView {
  BOOL isSoftWarning_;
}

- (instancetype)initWithFrame:(NSRect)frame isSoftWarning:(BOOL)isSoftWarning;

@end

@implementation PhishGuardDialogView

- (instancetype)initWithFrame:(NSRect)frame isSoftWarning:(BOOL)isSoftWarning {
  if ((self = [super initWithFrame:frame])) {
    isSoftWarning_ = isSoftWarning;

    NSBox* box = [[[NSBox alloc] initWithFrame:frame] autorelease];
    [box setFillColor:[NSColor whiteColor]];
    [box setBoxType:NSBoxCustom];
    [box setBorderType:NSNoBorder];
    [box setContentViewMargins:NSZeroSize];

    NSImageView* imageView = [[[NSImageView alloc]
        initWithFrame:NSMakeRect(20, 85, 50, 50)] autorelease];
    [imageView
        setImage:NSImageFromImageSkia(gfx::CreateVectorIcon(
                     vector_icons::kWarningIcon, 50, gfx::kGoogleRed700))];

    int titleTextId = isSoftWarning
                          ? IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY_SOFTER
                          : IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY;

    NSTextField* titleLabel =
        [TextFieldUtils titleWithString:l10n_util::GetNSString(titleTextId)];
    [titleLabel setFrame:NSMakeRect(93, 115, 390, 17)];

    NSTextField* messageLabel = [TextFieldUtils
        labelWithString:l10n_util::GetNSString(
                            IDS_PAGE_INFO_CHANGE_PASSWORD_DETAILS)];
    [messageLabel setFrame:NSMakeRect(93, 55, 375, 65)];
    cocoa_l10n_util::WrapOrSizeToFit(messageLabel);

    NSButton* changePasswordButton =
        [ButtonUtils buttonWithTitle:l10n_util::GetNSString(
                                         IDS_PAGE_INFO_CHANGE_PASSWORD_BUTTON)
                              action:@selector(changePassword:)
                              target:self];
    [changePasswordButton setFrame:NSMakeRect(235, 12, 150, 32)];
    [changePasswordButton setKeyEquivalent:kKeyEquivalentReturn];
    [changePasswordButton sizeToFit];

    NSButton* ignoreButton = [ButtonUtils
        buttonWithTitle:l10n_util::GetNSString(
                            IDS_PAGE_INFO_IGNORE_PASSWORD_WARNING_BUTTON)
                 action:@selector(ignore:)
                 target:self];
    [ignoreButton setFrame:NSMakeRect(370, 12, 100, 32)];
    [ignoreButton sizeToFit];

    [self addSubview:box];
    [self addSubview:imageView];
    [self addSubview:titleLabel];
    [self addSubview:messageLabel];
    [self addSubview:changePasswordButton];
    [self addSubview:ignoreButton];

    cocoa_l10n_util::VerticallyReflowGroup(
        @[ titleLabel, messageLabel, changePasswordButton, ignoreButton ]);
  }
  return self;
}

- (void)changePassword:(id)sender {
  [NSApp endSheet:[self window]];
}

- (void)ignore:(id)sender {
  [NSApp endSheet:[self window]];
}

@end

@implementation PhishGuardDialogController {
  NSWindow* parentWindow_;
  base::scoped_nsobject<NSWindow> window_;
}

- (instancetype)initWithParentWindow:(NSWindow*)window {
  if ((self = [super init])) {
    DCHECK(window);
    parentWindow_ = window;
  }

  return self;
}

- (NSWindow*)window {
  return window_.get();
}

- (void)windowWillClose:(NSNotification*)notification {
  [self autorelease];
}

- (void)showDialog:(BOOL)isSoftWarning {
  PhishGuardDialogView* view =
      [[[PhishGuardDialogView alloc] initWithFrame:NSMakeRect(0, 0, 480, 150)
                                     isSoftWarning:isSoftWarning] autorelease];

  window_.reset([[NSWindow alloc] initWithContentRect:[view frame]
                                            styleMask:NSTitledWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:YES]);
  [window_ setContentView:view];
  [window_ setBackgroundColor:[NSColor whiteColor]];

  [NSApp beginSheet:window_.get()
      modalForWindow:parentWindow_
       modalDelegate:self
      didEndSelector:@selector(didEndSheet:returnCode:contextInfo:)
         contextInfo:NULL];
}

- (void)didEndSheet:(NSWindow*)sheet
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  [sheet close];
  [NSApp stopModal];
}

@end
