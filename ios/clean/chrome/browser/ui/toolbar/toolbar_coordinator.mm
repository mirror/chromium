// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/toolbar/toolbar_coordinator.h"

#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/history_popup_commands.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_constants.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/clean/chrome/browser/ui/history_popup/history_popup_coordinator.h"
#import "ios/clean/chrome/browser/ui/omnibox/location_bar_coordinator.h"
#import "ios/clean/chrome/browser/ui/settings/settings_commands.h"
#import "ios/clean/chrome/browser/ui/settings/settings_coordinator.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_mediator.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_view_controller.h"
#import "ios/clean/chrome/browser/ui/tools/tools_coordinator.h"
#import "ios/clean/chrome/browser/ui/tools/tools_menu_commands.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolbarCoordinator ()<SettingsCommands,
                                 ToolsMenuCommands,
                                 TabHistoryPopupCommands>
// Location Bar contains the omnibox amongst other views.
@property(nonatomic, weak) LocationBarCoordinator* locationBarCoordinator;
// History Popup displays the forward or backward history in a popup menu.
@property(nonatomic, weak) HistoryPopupCoordinator* historyPopupCoordinator;
// Tools Menu coordinator.
@property(nonatomic, weak) ToolsCoordinator* toolsMenuCoordinator;
// The View Controller managed by this coordinator.
@property(nonatomic, strong) ToolbarViewController* viewController;
// The mediator owned by this coordinator.
@property(nonatomic, strong) ToolbarMediator* mediator;

@property(nonatomic, weak) SettingsCoordinator* settingsCoordinator;

@property(nonatomic, strong) CommandDispatcher* router;

@end

@implementation ToolbarCoordinator
@synthesize locationBarCoordinator = _locationBarCoordinator;
@synthesize historyPopupCoordinator = _historyPopupCoordinator;
@synthesize toolsMenuCoordinator = _toolsMenuCoordinator;
@synthesize viewController = _viewController;
@synthesize webState = _webState;
@synthesize mediator = _mediator;
@synthesize router = _router;
@synthesize settingsCoordinator = _settingsCoordinator;
@synthesize usesTabStrip = _usesTabStrip;

- (instancetype)init {
  if ((self = [super init])) {
    _mediator = [[ToolbarMediator alloc] init];
    _router = [[CommandDispatcher alloc] init];
  }
  return self;
}

// This coordinator uses a router, so return that.
- (id<NavigationCommands,
      TabGridCommands,
      TabHistoryPopupCommands,
      TabStripCommands,
      ToolsMenuCommands>)callableDispatcher {
  return static_cast<id>(self.router);
}

- (void)setWebState:(web::WebState*)webState {
  _webState = webState;
  self.mediator.webState = self.webState;
}

- (void)setUsesTabStrip:(BOOL)usesTabStrip {
  DCHECK(!self.started);
  _usesTabStrip = usesTabStrip;
}

#pragma mark - BrowserCoordinator

- (void)start {
  if (self.started)
    return;

  self.viewController = [[ToolbarViewController alloc]
      initWithDispatcher:self.callableDispatcher];
  self.viewController.usesTabStrip = self.usesTabStrip;

  [self.router startDispatchingToTarget:self
                            forSelector:@selector(showToolsMenu)];
  [self.router startDispatchingToTarget:self
                            forSelector:@selector(closeToolsMenu)];
  [self.router startDispatchingToTarget:self
                            forProtocol:@protocol(SettingsCommands)];

  [self.router startDispatchingToTarget:self.dispatcher
                            forProtocol:@protocol(NavigationCommands)];
  [self.router startDispatchingToTarget:self.dispatcher
                            forProtocol:@protocol(TabGridCommands)];
  [self.router startDispatchingToTarget:self.dispatcher
                            forProtocol:@protocol(TabStripCommands)];

  // Change this to be handled locally.
  [self.dispatcher startDispatchingToTarget:self
                                forProtocol:@protocol(TabHistoryPopupCommands)];
  [self.router startDispatchingToTarget:self.dispatcher
                            forProtocol:@protocol(TabHistoryPopupCommands)];

  self.mediator.consumer = self.viewController;
  self.mediator.webStateList = &self.browser->web_state_list();

  if (self.usesTabStrip) {
    [self.browser->broadcaster()
        addObserver:self.mediator
        forSelector:@selector(broadcastTabStripVisible:)];
  }
  LocationBarCoordinator* locationBarCoordinator =
      [[LocationBarCoordinator alloc] init];
  self.locationBarCoordinator = locationBarCoordinator;
  [self addChildCoordinator:locationBarCoordinator];
  [locationBarCoordinator start];
  [super start];
}

- (void)stop {
  [super stop];
  if (self.usesTabStrip) {
    [self.browser->broadcaster()
        removeObserver:self.mediator
           forSelector:@selector(broadcastTabStripVisible:)];
  }
  [self.router stopDispatchingToTarget:self];
  [self.router stopDispatchingToTarget:self.dispatcher];
  [self.dispatcher stopDispatchingToTarget:self];
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  // The location bar is contained.
  if (childCoordinator == self.locationBarCoordinator) {
    self.viewController.locationBarViewController =
        self.locationBarCoordinator.viewController;
    return;
  }

  // The history pop-up handles (for now) its own presentation, so it doesn't
  // need to do anything else.
  if (childCoordinator == self.historyPopupCoordinator)
    return;

  // All other children are presented.
  DCHECK(childCoordinator == self.settingsCoordinator ||
         childCoordinator == self.toolsMenuCoordinator);
  [self.viewController presentViewController:childCoordinator.viewController
                                    animated:YES
                                  completion:nil];
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  if (childCoordinator == self.toolsMenuCoordinator ||
      childCoordinator == self.settingsCoordinator) {
    [childCoordinator.viewController.presentingViewController
        dismissViewControllerAnimated:YES
                           completion:nil];
  }
}

#pragma mark - ToolsMenuCommands Implementation

- (void)showToolsMenu {
  ToolsCoordinator* toolsCoordinator = [[ToolsCoordinator alloc] init];
  [self addChildCoordinator:toolsCoordinator];
  // Pass in the router so the tools can correctly open the settings.
  toolsCoordinator.dispatcher = self.callableDispatcher;
  ToolsMenuConfiguration* menuConfiguration =
      [[ToolsMenuConfiguration alloc] initWithDisplayView:nil];
  menuConfiguration.inTabSwitcher = NO;
  menuConfiguration.noOpenedTabs = NO;
  menuConfiguration.inNewTabPage =
      (self.webState->GetLastCommittedURL() == GURL(kChromeUINewTabURL));

  toolsCoordinator.toolsMenuConfiguration = menuConfiguration;
  toolsCoordinator.webState = self.webState;
  self.toolsMenuCoordinator = toolsCoordinator;
  [toolsCoordinator start];
}

- (void)closeToolsMenu {
  [self.toolsMenuCoordinator stop];
  [self removeChildCoordinator:self.toolsMenuCoordinator];
}

#pragma mark - SettingsCommands

- (void)showSettings {
  SettingsCoordinator* settingsCoordinator = [[SettingsCoordinator alloc] init];
  [self addChildCoordinator:settingsCoordinator];
  // Pass in the router to Settings so it can close.
  settingsCoordinator.dispatcher = self.callableDispatcher;
  self.settingsCoordinator = settingsCoordinator;
  [settingsCoordinator start];
}

- (void)closeSettings {
  [self.settingsCoordinator stop];
  [self removeChildCoordinator:self.settingsCoordinator];
}

#pragma mark - HistoryPopupCommands Implementation

- (void)showTabHistoryPopupForBackwardHistory {
  if (!self.historyPopupCoordinator) {
    HistoryPopupCoordinator* historyPopupCoordinator =
        [[HistoryPopupCoordinator alloc] init];
    historyPopupCoordinator.positionProvider = self.viewController;
    historyPopupCoordinator.presentationProvider = self.viewController;
    historyPopupCoordinator.tabHistoryUIUpdater = self.viewController;
    historyPopupCoordinator.webState = self.webState;
    historyPopupCoordinator.presentingButton = ToolbarButtonTypeBack;
    historyPopupCoordinator.navigationItems =
        self.webState->GetNavigationManager()->GetBackwardItems();
    self.historyPopupCoordinator = historyPopupCoordinator;
    [self addChildCoordinator:self.historyPopupCoordinator];
  }
  [self.historyPopupCoordinator start];
}

- (void)showTabHistoryPopupForForwardHistory {
  if (!self.historyPopupCoordinator) {
    HistoryPopupCoordinator* historyPopupCoordinator =
        [[HistoryPopupCoordinator alloc] init];
    historyPopupCoordinator.positionProvider = self.viewController;
    historyPopupCoordinator.presentationProvider = self.viewController;
    historyPopupCoordinator.tabHistoryUIUpdater = self.viewController;
    historyPopupCoordinator.webState = self.webState;
    historyPopupCoordinator.presentingButton = ToolbarButtonTypeForward;
    historyPopupCoordinator.navigationItems =
        self.webState->GetNavigationManager()->GetForwardItems();
    self.historyPopupCoordinator = historyPopupCoordinator;
    [self addChildCoordinator:self.historyPopupCoordinator];
  }
  [self.historyPopupCoordinator start];
}

- (void)navigateToHistoryItem:(const web::NavigationItem*)item {
  DCHECK(item);
  int index = self.webState->GetNavigationManager()->GetIndexOfItem(item);
  DCHECK_GT(index, -1);
  self.webState->GetNavigationManager()->GoToIndex(index);
  [self.historyPopupCoordinator stop];
}

@end
