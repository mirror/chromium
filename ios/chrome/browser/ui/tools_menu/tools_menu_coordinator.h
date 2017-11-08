// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_H_

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_controller.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_constants.h"

// ToolsMenuCoordinator is a ChromeCoordinator that encapsulates logic for
// showing tools menu UI. In the typical case that may be a tools menu popup.
@interface ToolsMenuCoordinator
    : ChromeCoordinator<ToolsMenuCoordinatorPresentationStateProtocol>
// The dispatcher for this Coordinator. This Coordinator will register itself
// as the handler for tools menu commands (see the ToolsPopupCommands
// protocol) and will present and dismiss a tools popup in reaction to them.
@property(nonatomic, readwrite, strong) CommandDispatcher* dispatcher;

// A provider that prepares a configuration describing the contents of
// the tools popup menu list, as well as the state of other controls in the
// menu such as the Reload/Cancel Loading button.
@property(nonatomic, readwrite, weak)
    id<ToolsMenuCoordinatorConfigurationProvider>
        configurationProvider;

@property(nonatomic, readwrite, weak)
    id<ToolsMenuCoordinatorPresentationProvider>
        presentationProvider;

// Stops listening for dispatcher calls.
- (void)disconnect;

// Re-fetches configuration details from the
// ToolsPopupCoordinatorConfigurationProvider where it is possible for them
// to be changed after initialization (e.g. highlighting the Bookmark
// button in the popup if the current visible page is bookmarked).
- (void)updateConfiguration;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_H_
