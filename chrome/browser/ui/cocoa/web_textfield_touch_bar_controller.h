// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

@class AutofillPopupViewCocoa;
@class TabContentsController;
class WebTextfieldTouchBarBridgeObserver;

namespace content {
class WebContents;
}

// Provides a touch bar for the textfields in the WebContents. This class
// implements the NSTouchBarDelegate and handles the items in the touch bar.
@interface WebTextfieldTouchBarController : NSObject<NSTouchBarDelegate> {
  TabContentsController* owner_;       // weak.
  AutofillPopupViewCocoa* popupView_;  // weak.
  NSWindow* window_;                   // weak.
  std::unique_ptr<WebTextfieldTouchBarBridgeObserver> observer_;
  base::scoped_nsobject<NSString> textForSuggestions_;  // weak.
  NSMutableArray<NSTextCheckingResult*>* candidates_;
  API_AVAILABLE(macos(10.12.2)) NSCandidateListTouchBarItem* candidateListItem_;
  content::WebContents* contents_;
}

// Designated initializer.
- (instancetype)initWithTabContentsController:(TabContentsController*)owner
                                  webContents:(content::WebContents*)contents;

// Display the touch bar that is provided by |popupView|.
- (void)showCreditCardAutofillForPopupView:(AutofillPopupViewCocoa*)popupView;

- (void)updateTextSuggestionCandidateListWithText:(NSString*)text;

- (void)changeWebContents:(content::WebContents*)contents;

// Creates and returns a touch bar.
- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

@end

#endif  // CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_
