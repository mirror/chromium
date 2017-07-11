// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_WEB_TEXTFIELD_TOUCH_BAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "content/public/browser/focused_node_details.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"

class WebTextfieldTouchBarBridge;
@class TabContentsController;

namespace content {
struct FocusedNodeDetails;
class WebContents;
}  // namespace content

// Provides a touch bar for the browser window. This class implements the
// NSTouchBarDelegate and handles the items in the touch bar.
@interface WebTextfieldTouchBarController : NSObject<NSTouchBarDelegate> {
  std::unique_ptr<WebTextfieldTouchBarBridge> bridge_;
  TabContentsController* owner_;  // weak,
  BOOL isEditable_;
  content::WebContents* contents_;
}

// Designated initializer.
- (instancetype)initWithTabContentsController:(TabContentsController*)owner
                                  webContents:(content::WebContents*)contents;

- (void)changeWebContents:(content::WebContents*)contents;

- (void)updateFocusedNodeDetails:(BOOL)isEditable;

// Creates and returns a touch bar for the browser window.
- (NSTouchBar*)makeTouchBar;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_TOUCH_BAR_H_
