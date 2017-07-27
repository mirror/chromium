// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ELEMENT_FRAME_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ELEMENT_FRAME_PROVIDER_H_

// Defines an interface for accessing the frames of toolbar elements.
@protocol ToolbarElementFrameProvider

// Returns the frame of the tab switcher button.
- (CGRect)frameForTabSwitcherButton;

// Returns the frame of the tools menu button.
- (CGRect)frameForToolsMenuButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ELEMENT_FRAME_PROVIDER_H_
