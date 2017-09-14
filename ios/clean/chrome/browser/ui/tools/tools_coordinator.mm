// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tools/tools_coordinator.h"

#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/clean/chrome/browser/ui/settings/settings_commands.h"
#import "ios/clean/chrome/browser/ui/settings/settings_coordinator.h"
#import "ios/clean/chrome/browser/ui/tools/menu_view_controller.h"
#import "ios/clean/chrome/browser/ui/tools/tools_mediator.h"
#import "ios/clean/chrome/browser/ui/transitions/zooming_menu_transition_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolsCoordinator ()
@property(nonatomic, strong) ToolsMediator* mediator;
@property(nonatomic, strong)
    ZoomingMenuTransitionController* transitionController;
@property(nonatomic, strong) MenuViewController* viewController;
// Dispatcher for locally handled commands.
@property(nonatomic, strong) CommandDispatcher* router;
@end

@implementation ToolsCoordinator
@synthesize mediator = _mediator;
@synthesize transitionController = _transitionController;
@synthesize toolsMenuConfiguration = _toolsMenuConfiguration;
@synthesize viewController = _viewController;
@synthesize webState = _webState;
@synthesize router = _router;

#pragma mark - BrowserCoordinator

- (void)start {
  self.router = [[CommandDispatcher alloc] init];
  [self.router startDispatchingToTarget:self
                            forSelector:@selector(dismissToolsMenu)];
  [self.router startDispatchingToTarget:self
                            forSelector:@selector(showSettings)];
  self.viewController = [[MenuViewController alloc] init];
  self.viewController.modalPresentationStyle = UIModalPresentationCustom;
  self.transitionController = [[ZoomingMenuTransitionController alloc]
      initWithDispatcher:static_cast<id<ToolsMenuCommands>>(self.router)];
  self.viewController.transitioningDelegate = self.transitionController;
  self.viewController.dispatcher = self.callableDispatcher;
  self.viewController.menuDispatcher =
      static_cast<id<ToolsMenuCommands, SettingsCommands>>(self.router);
  self.mediator =
      [[ToolsMediator alloc] initWithConsumer:self.viewController
                                configuration:self.toolsMenuConfiguration];
  if (self.webState) {
    self.mediator.webState = self.webState;
  }
  [super start];
}

#pragma mark - Setters

- (void)setWebState:(web::WebState*)webState {
  _webState = webState;
  if (self.mediator) {
    self.mediator.webState = self.webState;
  }
}

#pragma mark - SettingsCommands

- (void)showSettings {
  SettingsCoordinator* settingsCoordinator = [[SettingsCoordinator alloc] init];
  [self.parentCoordinator addChildCoordinator:settingsCoordinator];
  [settingsCoordinator start];
  [self.parentCoordinator removeChildCoordinator:self];
}

#pragma mark - ToolsMenuCommands

- (void)dismissToolsMenu {
  [self stop];
}

@end
