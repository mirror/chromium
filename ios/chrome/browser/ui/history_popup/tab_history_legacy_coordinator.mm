// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history_popup/tab_history_legacy_coordinator.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/commands/history_popup_commands.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_factory.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_features.h"
#import "ios/chrome/browser/ui/fullscreen/scoped_fullscreen_disabler.h"
#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_constants.h"
#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_positioner.h"
#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_presentation.h"
#import "ios/chrome/browser/ui/history_popup/requirements/tab_history_ui_updater.h"
#import "ios/chrome/browser/ui/history_popup/tab_history_popup_controller.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using base::UserMetricsAction;

@interface LegacyTabHistoryCoordinator ()<PopupMenuDelegate> {
  // The disabler that prevents the toolbar from being hidden while the history
  // popup UI is displayed.
  std::unique_ptr<ScopedFullscreenDisabler> _fullscreenDisabler;
}

// The TabHistoryPopupController instance that this coordinator will be
// presenting.
@property(nonatomic, strong)
    TabHistoryPopupController* tabHistoryPopupController;

@end

@implementation LegacyTabHistoryCoordinator

@synthesize dispatcher = _dispatcher;
@synthesize browserState = _browserState;
@synthesize positionProvider = _positionProvider;
@synthesize presentationProvider = _presentationProvider;
@synthesize tabHistoryPopupController = _tabHistoryPopupController;
@synthesize tabHistoryUIUpdater = _tabHistoryUIUpdater;
@synthesize tabModel = _tabModel;

#pragma mark - Accessors

- (void)disconnect {
  self.dispatcher = nil;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher == self.dispatcher) {
    return;
  }
  if (self.dispatcher) {
    [self.dispatcher stopDispatchingToTarget:self];
  }
  [dispatcher startDispatchingToTarget:self
                           forProtocol:@protocol(TabHistoryPopupCommands)];
  _dispatcher = dispatcher;
}

- (void)setTabHistoryPopupController:
    (TabHistoryPopupController*)tabHistoryPopupController {
  if (_tabHistoryPopupController == tabHistoryPopupController)
    return;
  _tabHistoryPopupController = tabHistoryPopupController;
  if (base::FeatureList::IsEnabled(fullscreen::features::kNewFullscreen)) {
    FullscreenController* fullscreenController =
        self.browserState
            ? FullscreenControllerFactory::GetInstance()->GetForBrowserState(
                  self.browserState)
            : nullptr;
    _fullscreenDisabler =
        _tabHistoryPopupController && fullscreenController
            ? base::MakeUnique<ScopedFullscreenDisabler>(fullscreenController)
            : nullptr;
  }
}

#pragma mark - TabHistoryPopupCommands

- (void)showTabHistoryPopupForBackwardHistory {
  Tab* tab = [self.tabModel currentTab];
  web::NavigationItemList backwardItems =
      [tab navigationManager]->GetBackwardItems();
  CGPoint origin = [
      [self.presentationProvider viewForTabHistoryPresentation].window
      convertPoint:[self.positionProvider
                       originPointForToolbarButton:ToolbarButtonTypeBack]
            toView:[self.presentationProvider viewForTabHistoryPresentation]];

  [self.tabHistoryUIUpdater
      updateUIForTabHistoryPresentationFrom:ToolbarButtonTypeBack];
  [self presentTabHistoryPopupWithItems:backwardItems origin:origin];
}

- (void)showTabHistoryPopupForForwardHistory {
  Tab* tab = [self.tabModel currentTab];
  web::NavigationItemList forwardItems =
      [tab navigationManager]->GetForwardItems();
  CGPoint origin = [
      [self.presentationProvider viewForTabHistoryPresentation].window
      convertPoint:[self.positionProvider
                       originPointForToolbarButton:ToolbarButtonTypeForward]
            toView:[self.presentationProvider viewForTabHistoryPresentation]];

  [self.tabHistoryUIUpdater
      updateUIForTabHistoryPresentationFrom:ToolbarButtonTypeForward];
  [self presentTabHistoryPopupWithItems:forwardItems origin:origin];
}

- (void)navigateToHistoryItem:(const web::NavigationItem*)item {
  [[self.tabModel currentTab] goToItem:item];
  [self dismissPopupMenu:nil];
}

#pragma mark - Helper Methods

// Present a Tab History Popup that displays |items|, and its view is presented
// from |origin|.
- (void)presentTabHistoryPopupWithItems:(const web::NavigationItemList&)items
                                 origin:(CGPoint)historyPopupOrigin {
  if (self.tabHistoryPopupController)
    return;
  base::RecordAction(UserMetricsAction("MobileToolbarShowTabHistoryMenu"));

  [self.presentationProvider prepareForTabHistoryPresentation];

  // Initializing also displays the Tab History Popup VC.
  self.tabHistoryPopupController = [[TabHistoryPopupController alloc]
      initWithOrigin:historyPopupOrigin
          parentView:[self.presentationProvider viewForTabHistoryPresentation]
               items:items
          dispatcher:static_cast<id<TabHistoryPopupCommands>>(self.dispatcher)];

  [self.tabHistoryPopupController setDelegate:self];

  // Notify observers that TabHistoryPopup will show.
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kTabHistoryPopupWillShowNotification
                    object:nil];

  // Register to receive notification for when the App is backgrounded so we can
  // dismiss the TabHistoryPopup.
  NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self
                    selector:@selector(applicationDidEnterBackground:)
                        name:UIApplicationDidEnterBackgroundNotification
                      object:nil];
}

// Dismiss the presented Tab History Popup.
- (void)dismissHistoryPopup {
  if (!self.tabHistoryPopupController)
    return;
  __block TabHistoryPopupController* tempController =
      self.tabHistoryPopupController;
  [tempController containerView].userInteractionEnabled = NO;
  [tempController dismissAnimatedWithCompletion:^{
    [self.tabHistoryUIUpdater updateUIForTabHistoryWasDismissed];
    // Reference tempTHPC so the block retains it.
    tempController = nil;
  }];
  // Reset _tabHistoryPopupController to prevent -applicationDidEnterBackground
  // from posting another kTabHistoryPopupWillHideNotification.
  self.tabHistoryPopupController = nil;

  // Stop listening for notifications since these are only used to dismiss the
  // Tab History Popup.
  [[NSNotificationCenter defaultCenter] removeObserver:self];

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kTabHistoryPopupWillHideNotification
                    object:nil];
}

#pragma mark - PopupMenuDelegate

- (void)dismissPopupMenu:(PopupMenuController*)controller {
  [self dismissHistoryPopup];
}

#pragma mark - Observer Methods

- (void)applicationDidEnterBackground:(NSNotification*)notify {
  if (self.tabHistoryPopupController) {
    // Dismiss the tab history popup without animation.
    [self.tabHistoryUIUpdater updateUIForTabHistoryWasDismissed];
    self.tabHistoryPopupController = nil;
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kTabHistoryPopupWillHideNotification
                      object:nil];
  }
}

@end
