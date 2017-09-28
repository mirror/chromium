// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/tab/tab_coordinator.h"

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/scoped_observer.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#include "ios/chrome/browser/web_state_list/web_state_list.h"
#include "ios/chrome/browser/web_state_list/web_state_list_observer_bridge.h"
#import "ios/clean/chrome/browser/ui/commands/tab_commands.h"
#import "ios/clean/chrome/browser/ui/find_in_page/find_in_page_coordinator.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_controller.h"
#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_ui_updater.h"
#import "ios/clean/chrome/browser/ui/main_content/main_content_coordinator.h"
#import "ios/clean/chrome/browser/ui/main_content/main_content_view_controller.h"
#import "ios/clean/chrome/browser/ui/ntp/ntp_coordinator.h"
#import "ios/clean/chrome/browser/ui/settings/settings_coordinator.h"
#import "ios/clean/chrome/browser/ui/tab/tab_container_view_controller.h"
#include "ios/clean/chrome/browser/ui/tab/tab_features.h"
#import "ios/clean/chrome/browser/ui/tab/tab_navigation_controller.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_coordinator.h"
#import "ios/clean/chrome/browser/ui/transitions/zoom_transition_controller.h"
#import "ios/clean/chrome/browser/ui/web_contents/web_coordinator.h"
#import "ios/web/public/web_state/web_state.h"
#import "ios/web/public/web_state/web_state_observer_bridge.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabCoordinator ()<CRWWebStateObserver,
                             TabCommands,
                             WebStateListObserving> {
  std::unique_ptr<FullscreenUIUpdater> _fullscreenUIUpdater;
}

@property(nonatomic, strong) ZoomTransitionController* transitionController;
@property(nonatomic, strong) TabContainerViewController* viewController;
@property(nonatomic, weak) MainContentCoordinator* mainContentCoordinator;
@property(nonatomic, weak) NTPCoordinator* ntpCoordinator;
@property(nonatomic, weak) WebCoordinator* webCoordinator;
@property(nonatomic, weak) ToolbarCoordinator* toolbarCoordinator;
@property(nonatomic, strong) TabNavigationController* navigationController;

// Creates and returns a new view controller for use as a tab container.
// Subclasses may override this method with a custom layout view controller.
- (TabContainerViewController*)newTabContainer;

// Called when the main content coordinator starts or stops.
- (void)mainContentCoordinatorDidStart;
- (void)mainContentCoordinatorDidStop;

@end

@implementation TabCoordinator {
  std::unique_ptr<web::WebStateObserverBridge> _webStateObserver;
  std::unique_ptr<WebStateListObserverBridge> _webStateListObserver;
  std::unique_ptr<ScopedObserver<WebStateList, WebStateListObserverBridge>>
      _scopedWebStateListObserver;
}

@synthesize transitionController = _transitionController;
@synthesize presentationKey = _presentationKey;
@synthesize viewController = _viewController;
@synthesize webState = _webState;
@synthesize mainContentCoordinator = _mainContentCoordinator;
@synthesize webCoordinator = _webCoordinator;
@synthesize ntpCoordinator = _ntpCoordinator;
@synthesize toolbarCoordinator = _toolbarCoordinator;
@synthesize navigationController = _navigationController;

#pragma mark - Public

- (void)disconnect {
  _webStateListObserver.reset();
  _webStateObserver.reset();
}

#pragma mark - BrowserCoordinator

- (void)start {
  if (self.started)
    return;
  self.transitionController = [[ZoomTransitionController alloc] init];
  self.transitionController.presentationKey = self.presentationKey;
  self.viewController.transitioningDelegate = self.transitionController;
  self.viewController.modalPresentationStyle = UIModalPresentationCustom;
  _webStateObserver =
      base::MakeUnique<web::WebStateObserverBridge>(self.webState, self);

  _webStateListObserver = base::MakeUnique<WebStateListObserverBridge>(self);
  _scopedWebStateListObserver = base::MakeUnique<
      ScopedObserver<WebStateList, WebStateListObserverBridge>>(
      _webStateListObserver.get());
  _scopedWebStateListObserver->Add(&self.browser->web_state_list());

  [self.dispatcher startDispatchingToTarget:self
                                forSelector:@selector(loadURL:)];

  // Instantiate the FullscreenController and start forwarding events.
  FullscreenController::CreateForBrowser(self.browser);
  _fullscreenUIUpdater =
      base::MakeUnique<FullscreenUIUpdater>(self.viewController);
  FullscreenController::FromBrowser(self.browser)
      ->AddObserver(_fullscreenUIUpdater.get());

  [self.browser->broadcaster()
      broadcastValue:@"toolbarHeight"
            ofObject:self.viewController
            selector:@selector(broadcastToolbarHeight:)];

  // NavigationController will handle all the dispatcher navigation calls.
  self.navigationController = [[TabNavigationController alloc]
      initWithDispatcher:self.callableDispatcher
                webState:self.webState];

  WebCoordinator* webCoordinator = [[WebCoordinator alloc] init];
  webCoordinator.webState = self.webState;
  [self addChildCoordinator:webCoordinator];
  self.webCoordinator = webCoordinator;
  self.mainContentCoordinator = self.webCoordinator;

  ToolbarCoordinator* toolbarCoordinator = [[ToolbarCoordinator alloc] init];
  self.toolbarCoordinator = toolbarCoordinator;
  toolbarCoordinator.webState = self.webState;
  [self addChildCoordinator:toolbarCoordinator];
  [toolbarCoordinator start];

  // Create the FindInPage coordinator but do not start it.  It will be started
  // when a find in page operation is invoked.
  FindInPageCoordinator* findInPageCoordinator =
      [[FindInPageCoordinator alloc] init];
  [self addChildCoordinator:findInPageCoordinator];

  // PLACEHOLDER: Fix the order of events here. The ntpCoordinator was already
  // created above when |webCoordinator.webState = self.webState;| triggers
  // a load event, but then the webCoordinator stomps on the
  // contentViewController when it starts afterwards.
  self.mainContentCoordinator =
      self.ntpCoordinator ? self.ntpCoordinator : self.webCoordinator;

  [super start];
}

- (void)stop {
  [super stop];
  FullscreenController::FromBrowser(self.browser)
      ->RemoveObserver(_fullscreenUIUpdater.get());
  _fullscreenUIUpdater = nullptr;
  for (BrowserCoordinator* child in self.children)
    [self removeChildCoordinator:child];
  _scopedWebStateListObserver.reset();
  _webStateListObserver.reset();
  _webStateObserver.reset();
  [self.dispatcher stopDispatchingToTarget:self];
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastToolbarHeight:)];
  [self.navigationController stop];
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  if (childCoordinator == self.mainContentCoordinator) {
    [self mainContentCoordinatorDidStart];
  } else if ([childCoordinator isKindOfClass:[ToolbarCoordinator class]]) {
    self.viewController.toolbarViewController = childCoordinator.viewController;
  } else if ([childCoordinator isKindOfClass:[FindInPageCoordinator class]]) {
    self.viewController.findBarViewController = childCoordinator.viewController;
  } else if ([childCoordinator isKindOfClass:[SettingsCoordinator class]]) {
    [self.viewController presentViewController:childCoordinator.viewController
                                      animated:YES
                                    completion:nil];
  }
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  if (childCoordinator == self.mainContentCoordinator) {
    [self mainContentCoordinatorDidStop];
  } else if ([childCoordinator isKindOfClass:[FindInPageCoordinator class]]) {
    self.viewController.findBarViewController = nil;
  } else if ([childCoordinator isKindOfClass:[SettingsCoordinator class]]) {
    [childCoordinator.viewController.presentingViewController
        dismissViewControllerAnimated:YES
                           completion:nil];
  }
}

#pragma mark - Private accessors

- (TabContainerViewController*)viewController {
  if (!_viewController) {
    _viewController = [self newTabContainer];
    _viewController.usesBottomToolbar =
        base::FeatureList::IsEnabled(kTabFeaturesBottomToolbar);
  }
  return _viewController;
}

- (void)setMainContentCoordinator:
    (MainContentCoordinator*)mainContentCoordinator {
  if (_mainContentCoordinator == mainContentCoordinator)
    return;
  [_mainContentCoordinator stop];
  _mainContentCoordinator = mainContentCoordinator;
  [_mainContentCoordinator start];
}

#pragma mark - Methods for subclasses to override

- (TabContainerViewController*)newTabContainer {
  return [[TabContainerViewController alloc] init];
}

#pragma mark - CRWWebStateObserver

// This will eventually be called in -didFinishNavigation and perhaps as an
// optimization in some equivalent to loadURL.
- (void)webState:(web::WebState*)webState
    didCommitNavigationWithDetails:(const web::LoadCommittedDetails&)details {
  if (webState->GetLastCommittedURL() == GURL(kChromeUINewTabURL)) {
    [self addNTPCoordinator];
  }
}

- (void)webState:(web::WebState*)webState
    didStartNavigation:(web::NavigationContext*)navigation {
  [self removeNTPCoordinator];
}

#pragma mark - WebStateListObserver

- (void)webStateList:(WebStateList*)webStateList
    didChangeActiveWebState:(web::WebState*)newWebState
                oldWebState:(web::WebState*)oldWebState
                    atIndex:(int)atIndex
                 userAction:(BOOL)userAction {
  self.webState = newWebState;
  _webStateObserver =
      base::MakeUnique<web::WebStateObserverBridge>(self.webState, self);
  // Push down the new Webstate.
  self.navigationController.webState = newWebState;
  self.toolbarCoordinator.webState = newWebState;
  self.webCoordinator.webState = newWebState;

  if (self.webState->GetLastCommittedURL() == GURL(kChromeUINewTabURL)) {
    [self addNTPCoordinator];
  } else {
    [self removeNTPCoordinator];
  }
}

#pragma mark - Helper Methods

- (void)addNTPCoordinator {
  if (self.ntpCoordinator) {
    [self addChildCoordinator:self.ntpCoordinator];
  } else {
    NTPCoordinator* ntpCoordinator = [[NTPCoordinator alloc] init];
    [self addChildCoordinator:ntpCoordinator];
    self.ntpCoordinator = ntpCoordinator;
    self.mainContentCoordinator = ntpCoordinator;
  }
}

- (void)removeNTPCoordinator {
  if (self.ntpCoordinator) {
    [self.ntpCoordinator stop];
    [self removeChildCoordinator:self.ntpCoordinator];
    self.mainContentCoordinator = self.webCoordinator;
  }
}

- (void)mainContentCoordinatorDidStart {
  [self.browser->broadcaster()
      broadcastValue:@"yContentOffset"
            ofObject:_mainContentCoordinator.viewController
            selector:@selector(broadcastContentScrollOffset:)];
  [self.browser->broadcaster()
      broadcastValue:@"scrolling"
            ofObject:_mainContentCoordinator.viewController
            selector:@selector(broadcastScrollViewIsScrolling:)];
  [self.browser->broadcaster()
      broadcastValue:@"dragging"
            ofObject:_mainContentCoordinator.viewController
            selector:@selector(broadcastScrollViewIsDragging:)];
  self.viewController.contentViewController =
      self.mainContentCoordinator.viewController;
}

- (void)mainContentCoordinatorDidStop {
  self.viewController.contentViewController = nil;
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastContentScrollOffset:)];
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsScrolling:)];
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsDragging:)];
}

#pragma mark - TabCommands

- (void)loadURL:(web::NavigationManager::WebLoadParams)params {
  self.webState->GetNavigationManager()->LoadURLWithParams(params);
}

@end
