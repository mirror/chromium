// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/password_reuse_warning_dialog_controller.h"

#import "base/mac/scoped_nsobject.h"
#include "chrome/app/vector_icons/vector_icons.h"
#import "chrome/browser/ui/cocoa/chrome_style.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_control_utils.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#import "ui/base/cocoa/controls/imageview_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

// Size of the security icon.
constexpr CGFloat kIconSize = 24;

constexpr CGFloat kWindowMinWidth = 500;
constexpr CGFloat kButtonGap = 8;
constexpr CGFloat kIconGap = 6;

constexpr CGFloat kDialogAlertBarBorderWidth = 1;

// Determine the frame required to fit the content of a string.  Uses the
// provided height and width as preferred dimensions, where a value of
// 0.0 indicates no preference.
NSRect ComputeFrame(NSAttributedString* text, CGFloat width, CGFloat height) {
  NSRect frame =
      [text boundingRectWithSize:NSMakeSize(width, height)
                         options:NSStringDrawingUsesLineFragmentOrigin];
  // boundingRectWithSize is known to underestimate the width.
  static const CGFloat kTextViewPadding = 10;
  frame.size.width += kTextViewPadding;
  return frame;
}

}  // namespace

namespace safe_browsing {

void ShowPasswordReuseModalWarningDialog(content::WebContents* web_contents,
                                         OnWarningDone done_callback) {
  // Dialog owns itself.
  new PasswordReuseWarningDialogCocoa(web_contents, std::move(done_callback));
}

}  // namespace safe_browsing

PasswordReuseWarningDialogCocoa::PasswordReuseWarningDialogCocoa(
    content::WebContents* web_contents,
    safe_browsing::OnWarningDone callback)
    : callback_(std::move(callback)) {
  controller_.reset(
      [[PasswordReuseWarningViewController alloc] initWithOwner:this]);

  // Setup the constrained window that will show the view.
  base::scoped_nsobject<NSWindow> window([[ConstrainedWindowCustomWindow alloc]
      initWithContentRect:[[controller_ view] bounds]]);
  [[window contentView] addSubview:[controller_ view]];
  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
      [[CustomConstrainedWindowSheet alloc] initWithCustomWindow:window]);
  window_ = CreateAndShowWebModalDialogMac(this, web_contents, sheet);
}

PasswordReuseWarningDialogCocoa::~PasswordReuseWarningDialogCocoa() {}

void PasswordReuseWarningDialogCocoa::OnChangePassword() {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::CHANGE_PASSWORD);
  window_->CloseWebContentsModalDialog();
}

void PasswordReuseWarningDialogCocoa::OnIgnore() {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::IGNORE_WARNING);
  window_->CloseWebContentsModalDialog();
}

void PasswordReuseWarningDialogCocoa::OnConstrainedWindowClosed(
    ConstrainedWindowMac* window) {
  if (callback_)
    std::move(callback_).Run(safe_browsing::PasswordProtectionService::CLOSE);
}

@interface PasswordReuseWarningViewController () {
  base::scoped_nsobject<NSBox> box_;
  base::scoped_nsobject<NSTextField> titleField_;
  base::scoped_nsobject<NSImageView> iconView_;
  base::scoped_nsobject<NSTextField> messageField_;
  base::scoped_nsobject<ConstrainedWindowButton> ignoreButton_;
  base::scoped_nsobject<ConstrainedWindowButton> changePasswordButton_;
}

@end

@implementation PasswordReuseWarningViewController {
  base::scoped_nsobject<NSWindow> window_;
  PasswordReuseWarningDialogCocoa* owner_;
}

- (instancetype)initWithOwner:(PasswordReuseWarningDialogCocoa*)owner {
  if ((self = [super init])) {
    DCHECK(owner);
    owner_ = owner;
  }
  return self;
}

- (void)loadView {
  self.view = [[[NSView alloc] initWithFrame:NSZeroRect] autorelease];

  box_.reset([[NSBox alloc] initWithFrame:NSZeroRect]);
  [[self view] addSubview:box_];

  CGFloat dialogHeight = chrome_style::kClientBottomPadding;

  changePasswordButton_.reset(
      [[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  constrained_window::AddButton([self view], changePasswordButton_,
                                IDS_PAGE_INFO_CHANGE_PASSWORD_BUTTON, self,
                                @selector(changePassword:), true);
  [changePasswordButton_ setKeyEquivalent:kKeyEquivalentReturn];

  ignoreButton_.reset(
      [[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  constrained_window::AddButton([self view], ignoreButton_,
                                IDS_PAGE_INFO_IGNORE_PASSWORD_WARNING_BUTTON,
                                self, @selector(ignore:), true);

  dialogHeight += NSHeight([ignoreButton_ frame]) + chrome_style::kRowPadding;

  // Compute the dialog width using the title and buttons.
  const CGFloat buttonsWidth = NSWidth([changePasswordButton_ frame]) +
                               kButtonGap + NSWidth([ignoreButton_ frame]);
  const CGFloat titleWidth =
      NSWidth([titleField_ frame]) + kIconSize + kIconGap;

  // Dialog minimum width must include the padding.
  const CGFloat minWidth =
      kWindowMinWidth - 2 * chrome_style::kHorizontalPadding;
  const CGFloat width = std::max(minWidth, std::max(buttonsWidth, titleWidth));
  const CGFloat dialogWidth = width + 2 * chrome_style::kHorizontalPadding;

  CGFloat textLeftPadding =
      chrome_style::kHorizontalPadding + kIconSize + kIconGap;
  CGFloat textWidth = width - textLeftPadding;
  messageField_.reset([constrained_window::AddTextField(
      [self view],
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_CHANGE_PASSWORD_DETAILS),
      chrome_style::kTextFontStyle) retain]);

  [messageField_ setFrame:ComputeFrame([messageField_ attributedStringValue],
                                       textWidth, 0.0)];
  [messageField_ setFrameOrigin:NSMakePoint(textLeftPadding, dialogHeight)];
  dialogHeight += NSHeight([messageField_ frame]) + chrome_style::kRowPadding;

  BOOL isSoftWarning =
      safe_browsing::PasswordProtectionService::ShouldShowSofterWarning();
  int titleTextId = isSoftWarning ? IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY_SOFTER
                                  : IDS_PAGE_INFO_CHANGE_PASSWORD_SUMMARY;
  // Add the title label.
  titleField_.reset([constrained_window::AddTextField(
      [self view], l10n_util::GetStringUTF16(titleTextId),
      chrome_style::kTitleFontStyle) retain]);
  [titleField_
      setFrame:ComputeFrame([titleField_ attributedStringValue], 0.0, 0.0)];
  [titleField_ setFrameOrigin:NSMakePoint(textLeftPadding, dialogHeight)];
  dialogHeight += NSHeight([titleField_ frame]);

  SkColor iconColor = isSoftWarning ? gfx::kChromeIconGrey : gfx::kGoogleRed700;
  const gfx::VectorIcon& icon =
      isSoftWarning ? kSecurityIcon : vector_icons::kWarningIcon;
  iconView_.reset([ImageViewUtils
      imageViewWithImage:NSImageFromImageSkia(gfx::CreateVectorIcon(
                             icon, kIconSize, iconColor))]);
  [iconView_
      setFrame:NSMakeRect(chrome_style::kHorizontalPadding,
                          dialogHeight - kIconSize, kIconSize, kIconSize)];
  [self.view addSubview:iconView_.get()];

  dialogHeight += chrome_style::kRowPadding;

  // Prompt box.
  [box_ setBorderColor:[NSColor whiteColor]];
  [box_ setBorderWidth:kDialogAlertBarBorderWidth];
  [box_ setFillColor:[NSColor whiteColor]];
  [box_ setBoxType:NSBoxCustom];
  [box_ setTitlePosition:NSNoTitle];
  [box_ setFrame:NSMakeRect(0, 0, dialogWidth, dialogHeight)];

  // Layout the buttons and box based on the calculated width & height.
  CGFloat buttonX = dialogWidth - chrome_style::kHorizontalPadding -
                    NSWidth([changePasswordButton_ frame]);
  [changePasswordButton_
      setFrameOrigin:NSMakePoint(buttonX, chrome_style::kRowPadding)];

  buttonX -= kButtonGap;
  buttonX -= NSWidth([ignoreButton_ frame]);
  [ignoreButton_
      setFrameOrigin:NSMakePoint(buttonX, chrome_style::kRowPadding)];

  // Update the dialog frame with the computed dimensions.
  [[self view] setFrame:NSMakeRect(0, 0, dialogWidth, dialogHeight)];
}

- (void)changePassword:(id)sender {
  owner_->OnChangePassword();
}

- (void)ignore:(id)sender {
  owner_->OnIgnore();
}

@end
