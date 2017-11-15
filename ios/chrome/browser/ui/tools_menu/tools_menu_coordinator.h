// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_H_

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_controller.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_state_provider.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"

@protocol ToolsMenuConfigurationProvider
, ToolsMenuPresentationProvider;

// ToolsMenuCoordinator is a ChromeCoordinator that encapsulates logic for
// showing tools menu UI. In the typical case that may be a tools menu popup.
@interface ToolsMenuCoordinator
    : ChromeCoordinator<ToolsMenuPresentationStateProvider>
// The dispatcher for this Coordinator. This Coordinator will register itself
// as the handler for tools menu commands (see the ToolsPopupCommands
// protocol) and will present and dismiss a tools popup in reaction to them.
@property(nonatomic, readwrite, strong) CommandDispatcher* dispatcher;

// A provider that prepares a configuration describing the contents of
// the tools popup menu list, as well as the state of other controls in the
// menu such as the Reload/Cancel Loading button.
@property(nonatomic, readwrite, weak) id<ToolsMenuConfigurationProvider>
    configurationProvider;

// A provider that may provide more information about the manner in which
// the coordinator may be presented.
@property(nonatomic, readwrite, weak) id<ToolsMenuPresentationProvider>
    presentationProvider;

// A configuration that will be used when this coordinator presents a menu.
@property(nonatomic, readwrite, strong)
    ToolsMenuConfiguration* toolsMenuConfiguration;

// Influences how the tools menu shows the bookmark indicator in the tools UI.
@property(nonatomic, readwrite, assign) BOOL shouldHighlightBookmarkButton;

// Influences the presence of the find bar in the tools UI.
@property(nonatomic, readwrite, assign) BOOL shouldShowFindBar;

// Influences the presence of the share menu in the tools UI.
@property(nonatomic, readwrite, assign) BOOL shouldShowShareMenu;

// Influences how the tools menu shows page-reload related UI.
@property(nonatomic, readwrite, assign) BOOL tabIsLoading;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_COORDINATOR_H_
