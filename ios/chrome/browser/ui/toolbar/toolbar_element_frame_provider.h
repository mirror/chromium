// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

typedef NS_ENUM(NSInteger, ToolbarElement) {
  ToolbarElementTabSwitcher,
  ToolbarElementToolsMenuButton
};

@protocol ToolbarElementFrameProvider

- (CGRect)frameForToolbarElement:(ToolbarElement)toolbarElement;

@end
