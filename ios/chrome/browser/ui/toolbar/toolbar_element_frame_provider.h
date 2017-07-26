// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines an interface for accessing the frames of toolbar elements.
@protocol ToolbarElementFrameProvider

// Returns the frame of the tab switcher button.
- (CGRect)frameForTabSwitcherButton;

// Returns the frame of the tools menu button.
- (CGRect)frameForToolsMenuButton;

@end
