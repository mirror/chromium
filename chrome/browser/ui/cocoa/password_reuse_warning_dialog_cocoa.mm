// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/password_reuse_warning_dialog_cocoa.h"

#include <Carbon/Carbon.h>

#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/password_reuse_warning_view_controller.h"
#include "ui/base/material_design/material_design_controller.h"

namespace safe_browsing {

void ShowPasswordReuseModalWarningDialog(
    content::WebContents* web_contents,
    ChromePasswordProtectionService* service,
    OnWarningDone done_callback) {
  DCHECK(web_contents);

  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    chrome::ShowPasswordReuseWarningDialog(web_contents, service,
                                           std::move(done_callback));
    return;
  }

  // Dialog owns itself.
  new PasswordReuseWarningDialogCocoa(web_contents, service,
                                      std::move(done_callback));
}

}  // namespace safe_browsing

// A custom constrained window for the Password Reuse dialog.
@interface PasswordReuseWarningDialogCocoaWindow
    : ConstrainedWindowCustomWindow {
  PasswordReuseWarningDialogCocoa* owner_;

  // Controller for the dialog view.
  base::scoped_nsobject<PasswordReuseWarningViewController> controller_;
}

- (instancetype)initWithOwner:(PasswordReuseWarningDialogCocoa*)owner
                  contentRect:(NSRect)contentRect;

- (void)didEndSheet:(NSWindow*)sheet
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo;

@end

@implementation PasswordReuseWarningDialogCocoaWindow

- (instancetype)initWithOwner:(PasswordReuseWarningDialogCocoa*)owner
                  contentRect:(NSRect)contentRect {
  if ((self = [super initWithContentRect:contentRect])) {
    owner_ = owner;
  }

  return self;
}

- (void)didEndSheet:(NSWindow*)sheet
         returnCode:(int)returnCode
        contextInfo:(void*)contextInfo {
  owner_->DidEndSheet();
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  if (([event keyCode] == kVK_Escape) ||
      (([event keyCode] == kVK_ANSI_Period) &&
       (([event modifierFlags] & NSCommandKeyMask) != 0))) {
    owner_->Close();
    return YES;
  }
  return [super performKeyEquivalent:event];
}

@end

PasswordReuseWarningDialogCocoa::PasswordReuseWarningDialogCocoa(
    content::WebContents* web_contents,
    safe_browsing::ChromePasswordProtectionService* service,
    safe_browsing::OnWarningDone callback)
    : service_(service),
      url_(web_contents->GetLastCommittedURL()),
      callback_(std::move(callback)) {
  controller_.reset(
      [[PasswordReuseWarningViewController alloc] initWithOwner:this]);

  window_.reset([[PasswordReuseWarningDialogCocoaWindow alloc]
      initWithOwner:this
        contentRect:[[controller_ view] bounds]]);
  [[window_ contentView] addSubview:[controller_ view]];
  [NSApp beginSheet:window_.get()
      modalForWindow:web_contents->GetTopLevelNativeWindow()
       modalDelegate:window_.get()
      didEndSelector:@selector(didEndSheet:returnCode:contextInfo:)
         contextInfo:nullptr];

  if (service_)
    service_->AddObserver(this);
}

PasswordReuseWarningDialogCocoa::~PasswordReuseWarningDialogCocoa() {
  if (service_)
    service_->RemoveObserver(this);
}

void PasswordReuseWarningDialogCocoa::OnStartingGaiaPasswordChange() {
  Close();
}

void PasswordReuseWarningDialogCocoa::OnGaiaPasswordChanged() {
  Close();
}

void PasswordReuseWarningDialogCocoa::OnMarkingSiteAsLegitimate(
    const GURL& url) {
  if (url_.GetWithEmptyPath() == url.GetWithEmptyPath())
    Close();
}

void PasswordReuseWarningDialogCocoa::InvokeActionForTesting(
    safe_browsing::ChromePasswordProtectionService::WarningAction action) {
  switch (action) {
    case safe_browsing::ChromePasswordProtectionService::CHANGE_PASSWORD:
      OnChangePassword();
      break;
    case safe_browsing::ChromePasswordProtectionService::IGNORE_WARNING:
      OnIgnore();
      break;
    case safe_browsing::ChromePasswordProtectionService::CLOSE:
      Close();
      break;
    default:
      NOTREACHED();
      break;
  }
}

safe_browsing::ChromePasswordProtectionService::WarningUIType
PasswordReuseWarningDialogCocoa::GetObserverType() {
  return safe_browsing::ChromePasswordProtectionService::MODAL_DIALOG;
}

void PasswordReuseWarningDialogCocoa::OnChangePassword() {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::CHANGE_PASSWORD);
  Close();
}

void PasswordReuseWarningDialogCocoa::OnIgnore() {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::IGNORE_WARNING);
  Close();
}

void PasswordReuseWarningDialogCocoa::Close() {
  if (callback_)
    std::move(callback_).Run(safe_browsing::PasswordProtectionService::CLOSE);

  [NSApp endSheet:window_.get()];
}

void PasswordReuseWarningDialogCocoa::DidEndSheet() {
  [window_ close];
  [NSApp stopModal];
}
