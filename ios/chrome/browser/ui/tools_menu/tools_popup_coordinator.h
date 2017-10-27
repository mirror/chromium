// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_POPUP_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_POPUP_COORDINATOR_H_

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_controller.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"

@class ToolsPopupCoordinator;

// Notification that the tools menu will be shown.
extern NSString* const kToolsPopupMenuWillShowNotification;
// Notification that the tools menu will dismiss.
extern NSString* const kToolsPopupMenuWillHideNotification;
// Notification that the tools menu did show.
extern NSString* const kToolsPopupMenuDidShowNotification;
// Notification that the tools menu did dismiss.
extern NSString* const kToolsPopupMenuDidHideNotification;

@protocol ToolsPopupCoordinatorPresentationProvider<NSObject>
@optional
- (UIButton*)presentingButtonForPopupCoordinator:
    (ToolsPopupCoordinator*)coordinator;
@end

@protocol ToolsPopupCoordinatorConfigurationProvider<NSObject>
- (ToolsMenuConfiguration*)menuConfigurationForToolsPopupCoordinator:
    (ToolsPopupCoordinator*)coordinator;
@optional
- (BOOL)shouldHighlightBookmarkButtonForToolsPopupCoordinator:
    (ToolsPopupCoordinator*)coordinator;
- (BOOL)shouldShowFindBarForToolsPopupCoordinator:
    (ToolsPopupCoordinator*)coordinator;
- (BOOL)shouldShowShareMenuForToolsPopupCoordinator:
    (ToolsPopupCoordinator*)coordinator;
- (BOOL)tabIsLoadingForToolsPopupCoordinator:
    (ToolsPopupCoordinator*)coordinator;
@end

@interface ToolsPopupCoordinator : ChromeCoordinator
// The dispatcher for this Coordinator. This Coordinator will register itself
// as the handler for tools menu commands (see the ToolsPopupCommands
// protocol) and will present and dismiss a tools popup in reaction to them.
@property(nonatomic, readwrite, strong) CommandDispatcher* dispatcher;

// A provider that may provide information that affects the manner of
// presentation of the popup menu.
@property(nonatomic, readwrite, weak)
    id<ToolsPopupCoordinatorPresentationProvider>
        presentationProvider;

// A provider that prepares a configuration describing the contents of
// the tools popup menu list, as well as the state of other controls in the
// menu such as the Reload/Cancel Loading button.
@property(nonatomic, readwrite, weak)
    id<ToolsPopupCoordinatorConfigurationProvider>
        configurationProvider;

// Returns whether the tools menu popup is currently being presented.
@property(nonatomic, readonly) BOOL isShowingToolsMenuPopup;

// Stops listening for dispatcher calls.
- (void)disconnect;

// Re-fetches configuration details from the
// ToolsPopupCoordinatorConfigurationProvider where it is possible for them
// to be changed after initialization (e.g. highlighting the Bookmark
// button in the popup if the current visible page is bookmarked).
- (void)updateConfiguration;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_POPUP_COORDINATOR_H_
