// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_

#import <Cocoa/Cocoa.h>

#include "ui/base/cocoa/touch_bar_forward_declarations.h"

@protocol SeparateFullscreenWindowDelegate
- (NSTouchBar*)touchBarForFullscreenWindow API_AVAILABLE(macos(10.12.2));
@end

@interface SeparateFullscreenWindow : NSWindow

@property(nonatomic, assign) id<SeparateFullscreenWindowDelegate> controller;

- (NSTouchBar*)makeTouchBar API_AVAILABLE(macos(10.12.2));

@end

#endif  // CHROME_BROWSER_UI_COCOA_SEPARATE_FULLSCREEN_WINDOW_H_
