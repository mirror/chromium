// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/touch_bar_forward_declarations.h"

#include "chrome/browser/ui/cocoa/browser_window_controller.h"

@interface SeparateFullscreenWindow : NSWindow {
 @private
  BrowserWindowController* controller_;
}

- (void)setController:(BrowserWindowController*)controller;

- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

@end

#endif  // CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
