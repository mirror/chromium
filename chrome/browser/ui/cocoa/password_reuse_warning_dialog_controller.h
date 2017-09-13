// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"

@class PasswordReuseWarningViewController;

class PasswordReuseWarningDialogCocoa : ConstrainedWindowMacDelegate {
 public:
  PasswordReuseWarningDialogCocoa(content::WebContents* web_contents,
                                  safe_browsing::OnWarningDone callback);

  ~PasswordReuseWarningDialogCocoa();

  void OnChangePassword();
  void OnIgnore();

 private:
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override;

  safe_browsing::OnWarningDone callback_;
  base::scoped_nsobject<PasswordReuseWarningViewController> controller_;
  std::unique_ptr<ConstrainedWindowMac> window_;

  DISALLOW_COPY_AND_ASSIGN(PasswordReuseWarningDialogCocoa);
};

// Class that acts as a controller for the modal PhishGuard dialog.
@interface PasswordReuseWarningViewController : NSViewController

- (instancetype)initWithOwner:(PasswordReuseWarningDialogCocoa*)owner;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_
