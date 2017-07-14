// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#import "ui/base/cocoa/touch_bar_forward_declarations.h"

@class TabContentsController;

namespace autofill {
class AutofillPopupController;
}

// Provides a touch bar for the browser window. This class implements the
// NSTouchBarDelegate and handles the items in the touch bar.
@interface WebTextfieldTouchBarController : NSObject<NSTouchBarDelegate> {
  TabContentsController* owner_;                        // weak.
  autofill::AutofillPopupController* popupController_;  // weak.
}

// Designated initializer.
- (instancetype)initWithTabContentsController:(TabContentsController*)owner;

- (void)showCreditCardAutofillForWindow:(NSWindow*)window
                             controller:
                                 (autofill::AutofillPopupController*)controller;

// Creates and returns a touch bar for the browser window.
- (NSTouchBar*)makeTouchBar;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_TOUCH_BAR_H_
