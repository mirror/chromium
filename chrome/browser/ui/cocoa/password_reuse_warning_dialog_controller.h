// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"

class PasswordReuseCallbackBridge {};

// Class that acts as a controller for the modal PhishGuard dialog.
@interface PasswordReuseWarningDialogController : NSWindowController

- (instancetype)initWithParentWindow:(NSWindow*)window;

- (void)showDialogWithCallback:(safe_browsing::OnWarningDone)callback;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_CONTROLLER_H_
