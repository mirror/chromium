// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/password_reuse_warning_dialog_controller.h"

#import "base/mac/scoped_nsobject.h"
#include "chrome/app/vector_icons/vector_icons.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#import "ui/base/cocoa/controls/button_utils.h"
#import "ui/base/cocoa/controls/imageview_utils.h"
#import "ui/base/cocoa/controls/textfield_utils.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

// Size of the security icon.
constexpr int kIconSize = 24;

// Margin values at the edges of the bubble.
constexpr int kHorizontalMargin = 16;
constexpr int kVerticalMargin = 16;

// Width of the dialog.
constexpr int kDialogWidth = 512;
}

namespace safe_browsing {

void ShowPasswordReuseModalWarningDialog(content::WebContents* web_contents,
                                         OnWarningDone done_callback) {
  BrowserWindowController* bwc = [BrowserWindowController
      browserWindowControllerForWindow:web_contents->GetTopLevelNativeWindow()];
  [bwc showPasswordReuseForWebContents:web_contents
                              callback:std::move(done_callback)];
}

}  //  namespace

@implementation PasswordReuseWarningDialogController {
  NSWindow* parentWindow_;
  base::scoped_nsobject<NSWindow> window_;
  safe_browsing::OnWarningDone callback_;
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

- (void)showDialogWithCallback:(safe_browsing::OnWarningDone)callback {
  callback_ = std::move(callback);

  [self setUpWindow];

  [NSApp beginSheet:window_.get()
      modalForWindow:parentWindow_
       modalDelegate:self
      didEndSelector:@selector(didEndSheet:returnCode:contextInfo:)
         contextInfo:NULL];
}

- (void)setUpWindow {
  BOOL isSoftWarning =
      safe_browsing::PasswordProtectionService::ShouldShowSofterWarning();

  int dialogHeight = kVerticalMargin;

  NSButton* ignoreButton = [ButtonUtils
      buttonWithTitle:l10n_util::GetNSString(
                          IDS_PAGE_INFO_IGNORE_PASSWORD_WARNING_BUTTON)
               action:@selector(ignore:)
               target:self];
  [ignoreButton sizeToFit];

  NSRect ignoreButtonFrame = [ignoreButton frame];
  ignoreButtonFrame.origin.x =
      kDialogWidth - kHorizontalMargin - NSWidth([ignoreButton frame]);
  ignoreButtonFrame.origin.y = kVerticalMargin;
  [ignoreButton setFrame:ignoreButtonFrame];

  NSButton* changePasswordButton =
      [ButtonUtils buttonWithTitle:l10n_util::GetNSString(
                                       IDS_PAGE_INFO_CHANGE_PASSWORD_BUTTON)
                            action:@selector(changePassword:)
                            target:self];
  [changePasswordButton setKeyEquivalent:kKeyEquivalentReturn];
  [changePasswordButton sizeToFit];

  NSRect changePasswordButtonFrame = [changePasswordButton frame];
  changePasswordButtonFrame.origin.x =
      NSMinX([ignoreButton frame]) - NSWidth(changePasswordButtonFrame) - 8;
  changePasswordButtonFrame.origin.y = kVerticalMargin;
  [changePasswordButton setFrame:changePasswordButtonFrame];

  dialogHeight += NSHeight([changePasswordButton frame]) + 16;

  NSTextField* messageLabel = [TextFieldUtils
      labelWithString:l10n_util::GetNSString(
                          IDS_PAGE_INFO_CHANGE_PASSWORD_DETAILS)];
  [messageLabel setFrame:NSMakeRect(56, dialogHeight, 440, 65)];
  cocoa_l10n_util::WrapOrSizeToFit(messageLabel);

  dialogHeight += NSHeight([messageLabel frame]) + 8;

  int titleTextId = isSoftWarning ? IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY_SOFTER
                                  : IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY;

  NSTextField* titleLabel =
      [TextFieldUtils titleWithString:l10n_util::GetNSString(titleTextId)];
  [titleLabel setFrame:NSMakeRect(56, dialogHeight, 440, 17)];
  cocoa_l10n_util::WrapOrSizeToFit(titleLabel);

  dialogHeight += NSHeight([titleLabel frame]);

  SkColor iconColor = isSoftWarning ? gfx::kChromeIconGrey : gfx::kGoogleRed700;
  const gfx::VectorIcon& icon =
      isSoftWarning ? kSecurityIcon : vector_icons::kWarningIcon;

  NSImageView* imageView = [ImageViewUtils
      imageViewWithImage:NSImageFromImageSkia(gfx::CreateVectorIcon(
                             icon, kIconSize, iconColor))];
  [imageView setFrame:NSMakeRect(kHorizontalMargin, dialogHeight - kIconSize,
                                 kIconSize, kIconSize)];
  dialogHeight += kVerticalMargin;

  NSRect dialogFrame = NSMakeRect(0, 0, kDialogWidth, dialogHeight);

  NSBox* box = [[[NSBox alloc] initWithFrame:dialogFrame] autorelease];
  [box setFillColor:[NSColor whiteColor]];
  [box setBoxType:NSBoxCustom];
  [box setBorderType:NSNoBorder];
  [box setContentViewMargins:NSZeroSize];

  NSView* contentView =
      [[[NSView alloc] initWithFrame:dialogFrame] autorelease];

  [contentView addSubview:box];
  [contentView addSubview:imageView];
  [contentView addSubview:titleLabel];
  [contentView addSubview:messageLabel];
  [contentView addSubview:changePasswordButton];
  [contentView addSubview:ignoreButton];

  cocoa_l10n_util::FlipAllSubviewsIfNecessary(contentView);

  window_.reset([[NSWindow alloc] initWithContentRect:dialogFrame
                                            styleMask:NSTitledWindowMask
                                              backing:NSBackingStoreBuffered
                                                defer:YES]);
  [window_ setContentView:contentView];
}

- (void)changePassword:(id)sender {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::CHANGE_PASSWORD);
  [NSApp endSheet:[self window]];
}

- (void)ignore:(id)sender {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::IGNORE_WARNING);
  [NSApp endSheet:[self window]];
}

- (void)didEndSheet:(NSWindow*)sheet
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  if (callback_)
    std::move(callback_).Run(safe_browsing::PasswordProtectionService::CLOSE);

  [sheet close];
  [NSApp stopModal];
}

@end
