// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/separate_fullscreen_window.h"
#import "chrome/browser/ui/cocoa/browser_window_touch_bar.h"

@implementation SeparateFullscreenWindow

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

- (void)setController:(BrowserWindowController*)controller {
  controller_ = controller;
}

- (NSTouchBar*)makeTouchBar {
  if (controller_) {
    return [[controller_ browserWindowTouchBar] createTabFullscreenTouchBar];
  }
  return nullptr;
}

@end
