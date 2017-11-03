// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONFIGURATION_VISIBILITY_PROTOCOL_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONFIGURATION_VISIBILITY_PROTOCOL_H_

#import <Foundation/Foundation.h>

// Protocol defining behaviors that must be implemented by the provided
// ToolsMenuButton
@protocol ToolsMenuConfigurationVisibilityProtocol
// Inform the receiver of the visibility of the tools menu. |menuIsVisible|
// should be YES if the tools menu has been shown, and NO if the tools menu
// has been hidden.
- (void)setToolsMenuIsVisible:(BOOL)menuIsVisible;
- (BOOL)toolsMenuIsVisible;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONFIGURATION_VISIBILITY_PROTOCOL_H_
