// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PHISH_GUARD_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_PHISH_GUARD_DIALOG_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

@class PhishGuardDialogView;

// Class that acts as a controller for the modal PhishGuard dialog.
@interface PhishGuardDialogController : NSWindowController

- (instancetype)initWithParentWindow:(NSWindow*)window;

- (void)showDialog:(BOOL)isSoftWarning;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PHISH_GUARD_DIALOG_CONTROLLER_H_
