// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/separate_fullscreen_window.h"

#include "chrome/browser/ui/cocoa/browser_window_touch_bar.h"

@implementation SeparateFullscreenWindow

@synthesize touch_bar_delegate = touch_bar_delegate_;

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

- (NSTouchBar*)makeTouchBar {
  return [touch_bar_delegate_ touchBarForFullscreenWindow];
}

@end
