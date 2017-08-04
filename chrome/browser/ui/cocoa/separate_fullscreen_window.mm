// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/separate_fullscreen_window.h"

#include "chrome/browser/ui/cocoa/browser_window_touch_bar.h"

@implementation SeparateFullscreenWindow

@synthesize controller = controller_;

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

- (NSTouchBar*)makeTouchBar {
  return [controller_ touchBarForFullscreenWindow];
}

@end
