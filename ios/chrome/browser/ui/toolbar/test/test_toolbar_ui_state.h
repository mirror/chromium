// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TEST_TEST_TOOLBAR_UI_STATE_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TEST_TEST_TOOLBAR_UI_STATE_H_

#import "ios/chrome/browser/ui/toolbar/toolbar_ui_state.h"

// A test version of ToolbarUIState that can be updated directly without the use
// of a ToolbarUIStateUpdater.
@interface TestToolbarUIState : ToolbarUIState

// Redefine broadcast property as readwrite.
@property(nonatomic, assign) CGFloat toolbarHeight;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TEST_TEST_TOOLBAR_UI_STATE_H_
