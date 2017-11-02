// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_CONSTANTS_H_

#import <Foundation/Foundation.h>

@class ToolsMenuCoordinator, ToolsMenuConfiguration;

// Notification that the tools menu will be shown.
extern NSString* const kToolsMenuWillShowNotification;
// Notification that the tools menu will dismiss.
extern NSString* const kToolsMenuWillHideNotification;
// Notification that the tools menu did show.
extern NSString* const kToolsMenuDidShowNotification;
// Notification that the tools menu did dismiss.
extern NSString* const kToolsMenuDidHideNotification;

// A protocol which allows objects to be able to request information about
// the presentation state of the tools UI without having to have full
// knowledge of the ToolsMenuCoordinator class.
@protocol ToolsMenuCoordinatorPresentationStateProtocol<NSObject>
// Returns whether the tools menu on screen and being presented to the user.
- (BOOL)isShowingToolsMenu;
@end

// A protocol that describes a set of methods which may configure a
// ToolsMenuCoordinator. Most of the configuration of the coordinator may be
// achieved through the required ToolsMenuConfiguration object, but some
// optional minor elements such as bookmark highlights are also independently
// configurable.
@protocol ToolsMenuCoordinatorConfigurationProvider<NSObject>
// Returns a ToolsMenuConfiguration object describing the desired configuration
// of the tools menu.
- (ToolsMenuConfiguration*)menuConfigurationForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@optional
// If implemented, may influence how the tools menu shows the bookmark
// indicator in the tools UI.
- (BOOL)shouldHighlightBookmarkButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented, may influence the presence of the find bar in the tools
// UI.
- (BOOL)shouldShowFindBarForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented,may influence the presence of the share menu in the tools
// UI.
- (BOOL)shouldShowShareMenuForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented, may influence how the tools menu shows page-reload related
// UI.
- (BOOL)tabIsLoadingForToolsMenuCoordinator:(ToolsMenuCoordinator*)coordinator;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_CONSTANTS_H_
