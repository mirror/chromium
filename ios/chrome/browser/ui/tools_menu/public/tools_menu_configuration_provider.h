// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_CONFIGURATION_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_CONFIGURATION_PROVIDER_H_

#import <Foundation/Foundation.h>

@class ToolsMenuCoordinator, ToolsMenuConfiguration;

// A protocol that describes a set of methods which may configure a
// ToolsMenuCoordinator. Most of the configuration of the coordinator may be
// achieved through the required ToolsMenuConfiguration object, but some
// optional minor elements such as bookmark highlights are also independently
// configurable.
@protocol ToolsMenuConfigurationProvider<NSObject>
// If implemented, allows the delegate to be informed of an imminent
// presentation of the tools menu. The delegate may choose to dismiss other
// presented UI or otherwise configure itself for the menu presentation.
- (void)prepareForToolsMenuPresentationByCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_PUBLIC_TOOLS_MENU_CONFIGURATION_PROVIDER_H_
