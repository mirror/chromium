// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_container_coordinator.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#include "ios/chrome/browser/ui/browser_list/browser_list.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_factory.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/clean/chrome/browser/ui/settings/settings_commands.h"
#import "ios/clean/chrome/browser/ui/settings/settings_coordinator.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_container_view_controller.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_coordinator.h"
#import "ios/clean/chrome/browser/ui/tab_grid/tab_grid_toolbar_commands.h"
#import "ios/clean/chrome/browser/ui/tools/tools_coordinator.h"
#import "ios/clean/chrome/browser/ui/tools/tools_menu_commands.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabGridContainerCoordinator ()<ToolsMenuCommands,
                                          TabGridToolbarCommands>

// Tab grid handling the non-incognito tabs.
@property(nonatomic, strong) TabGridCoordinator* normalTabGrid;
// Tab grid handling the incognito tabs.
@property(nonatomic, strong) TabGridCoordinator* incognitoTabGrid;

// ViewController containing the toolbar and the currently presented TabGrid.
@property(nonatomic, strong) TabGridContainerViewController* viewController;

// Coordinator handling the settings.
@property(nonatomic, weak) SettingsCoordinator* settingsCoordinator;
// Coordinator handling the toolmenu.
@property(nonatomic, weak) ToolsCoordinator* toolsMenuCoordinator;

@property(nonatomic, assign) BOOL incognito;

// Dispatcher for locally handled commands.
@property(nonatomic, strong) CommandDispatcher* router;

@end

@implementation TabGridContainerCoordinator

@synthesize viewController = _viewController;
@synthesize settingsCoordinator = _settingsCoordinator;
@synthesize toolsMenuCoordinator = _toolsMenuCoordinator;
@synthesize normalTabGrid = _normalTabGrid;
@synthesize incognitoTabGrid = _incognitoTabGrid;
@synthesize incognito = _incognito;
@synthesize router = _router;

- (void)start {
  if (self.started)
    return;

  self.router = [[CommandDispatcher alloc] init];
  [self.router startDispatchingToTarget:self
                            forProtocol:@protocol(ToolsMenuCommands)];
  [self.router startDispatchingToTarget:self
                            forProtocol:@protocol(TabGridToolbarCommands)];

  self.viewController = [[TabGridContainerViewController alloc] init];
  self.viewController.dispatcher =
      static_cast<id<TabGridToolbarCommands, ToolsMenuCommands>>(self.router);
  self.normalTabGrid = [[TabGridCoordinator alloc] init];
  [self addChildCoordinator:self.normalTabGrid];
  [self.normalTabGrid start];
  self.incognito = NO;

  [super start];
}

- (void)stop {
  [super stop];
  [self.router stopDispatchingToTarget:self];
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  if (childCoordinator == self.normalTabGrid ||
      childCoordinator == self.incognitoTabGrid) {
    self.viewController.tabGrid = childCoordinator.viewController;
  } else {
    [super childCoordinatorDidStart:childCoordinator];
  }
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  if (!(childCoordinator == self.normalTabGrid ||
        childCoordinator == self.incognitoTabGrid)) {
    [super childCoordinatorWillStop:childCoordinator];
  }
}

#pragma mark - Property

- (void)setIncognito:(BOOL)incognito {
  _incognito = incognito;
  self.viewController.incognito = incognito;
}

#pragma mark - URLOpening

- (void)openURL:(NSURL*)URL {
  [self.normalTabGrid openURL:URL];
}

#pragma mark-- ToolsMenuCommands

- (void)showToolsMenu {
  ToolsCoordinator* toolsCoordinator = [[ToolsCoordinator alloc] init];
  [self addChildCoordinator:toolsCoordinator];
  ToolsMenuConfiguration* menuConfiguration =
      [[ToolsMenuConfiguration alloc] initWithDisplayView:nil];
  menuConfiguration.inTabSwitcher = YES;
  menuConfiguration.noOpenedTabs = self.browser->web_state_list().empty();
  menuConfiguration.inNewTabPage = NO;
  toolsCoordinator.toolsMenuConfiguration = menuConfiguration;
  self.toolsMenuCoordinator = toolsCoordinator;
  [toolsCoordinator start];
}

- (void)closeToolsMenu {
  [self.toolsMenuCoordinator stop];
  [self removeChildCoordinator:self.toolsMenuCoordinator];
}

#pragma mark - TabGridToolbarCommands

- (void)toggleIncognito {
  if (self.incognito) {
    [self.incognitoTabGrid stop];
    [self.normalTabGrid start];
  } else {
    if (!self.incognitoTabGrid) {
      self.incognitoTabGrid = [[TabGridCoordinator alloc] init];
      [self addChildCoordinator:self.incognitoTabGrid];
      self.incognitoTabGrid.browser =
          BrowserListFactory::GetForBrowserState(
              self.browser->browser_state()
                  ->GetOffTheRecordChromeBrowserState())
              ->CreateNewBrowser();
    }
    [self.normalTabGrid stop];
    [self.incognitoTabGrid start];
  }
  self.incognito = !self.incognito;
}

#pragma mark - SettingsCommands

- (void)showSettings {
  SettingsCoordinator* settingsCoordinator = [[SettingsCoordinator alloc] init];
  [self addChildCoordinator:settingsCoordinator];
  self.settingsCoordinator = settingsCoordinator;
  [settingsCoordinator start];
}

- (void)closeSettings {
  [self.settingsCoordinator stop];
  [self removeChildCoordinator:self.settingsCoordinator];
}

@end
