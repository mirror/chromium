// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_H_

#import <Foundation/Foundation.h>

@class ToolbarUIState;

// Protocol for the UI displaying the toolbar.
@protocol ToolbarUI<NSObject>

// The current state of the toolbar UI.
@property(nonatomic, readonly) ToolbarUIState* toolbarUIState;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_UI_H_
