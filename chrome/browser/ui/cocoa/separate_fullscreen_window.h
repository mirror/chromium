// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/touch_bar_forward_declarations.h"

#include "chrome/browser/ui/cocoa/browser_window_controller.h"
// #include "chrome/browser/ui/browser.h"
// #include "chrome/browser/ui/tabs/tab_strip_model.h"
// #include "content/public/browser/web_contents.h"
// #include "ui/base/models/list_selection_model.h"

/*namespace content {
  class WebContents;
}

class TabStripModel;*/

@interface SeparateFullscreenWindow : NSWindow {
 @private
  BrowserWindowController* controller_;
  // TabStripModel* tabStripModel;
  // content::WebContents* tabContents;
  // content::WebContents* oldContents;
  // Browser* browser_;
}

- (BOOL)canBecomeKeyWindow;

- (BOOL)canBecomeMainWindow;

// - (void)setTabStripModel:(TabStripModel*)model;

// This will be used when the separate fullscreen window is closed to
// - (void)setWebContents:(content::WebContents*)tab;
//
// - (void)setBrowser:(Browser*)browser;

- (void)setController:(BrowserWindowController*)controller;

- (NSTouchBar*)makeTouchBar;

@end

#endif  // CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
