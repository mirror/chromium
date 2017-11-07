// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_coordinator.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/ui/commands/tools_popup_commands.h"
#import "ios/chrome/browser/ui/tools_menu/tools_popup_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolsMenuCoordinator ()<ToolsPopupCommands, PopupMenuDelegate> {
  // The following is nil if not visible.
  ToolsPopupController* toolsPopupController_;
}
@end

@implementation ToolsMenuCoordinator
@synthesize configurationProvider = configurationProvider_;
@synthesize presentationProvider = presentationProvider_;
@synthesize dispatcher = _dispatcher;

- (void)disconnect {
  self.dispatcher = nil;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher != self.dispatcher) {
    if (self.dispatcher) {
      [self.dispatcher stopDispatchingToTarget:self];
    }
    if (dispatcher) {
      [dispatcher startDispatchingToTarget:self
                               forProtocol:@protocol(ToolsPopupCommands)];
    }
    _dispatcher = dispatcher;
  }
}

- (void)showToolsMenu {
  ToolsMenuConfiguration* configuration = [self.configurationProvider
      menuConfigurationForToolsMenuCoordinator:self];

  if ([self.presentationProvider respondsToSelector:@selector
                                 (presentingButtonForToolsMenuCoordinator:)]) {
    configuration.toolsMenuButton = [self.presentationProvider
        presentingButtonForToolsMenuCoordinator:self];
  }

  [self showToolsMenuPopupWithConfiguration:configuration
                                 dispatcher:self.dispatcher];
}

- (void)showToolsMenuPopupWithConfiguration:
            (ToolsMenuConfiguration*)configuration
                                 dispatcher:(CommandDispatcher*)dispatcher {
  // Because an animation hides and shows the tools popup menu it is possible to
  // tap the tools button multiple times before the tools menu is shown. Ignore
  // repeated taps between animations.
  if ([self isShowingToolsMenu])
    return;

  base::RecordAction(base::UserMetricsAction("ShowAppMenu"));

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kToolsMenuWillShowNotification
                    object:self];
  if ([self.configurationProvider
          respondsToSelector:@selector
          (prepareForToolsMenuPresentationByCoordinator:)]) {
    [self.configurationProvider
        prepareForToolsMenuPresentationByCoordinator:self];
  }

  toolsPopupController_ = [[ToolsPopupController alloc]
      initAndPresentWithConfiguration:configuration
                           dispatcher:(id<ApplicationCommands, BrowserCommands>)
                                          dispatcher
                           completion:^{
                             [[NSNotificationCenter defaultCenter]
                                 postNotificationName:
                                     kToolsMenuDidShowNotification
                                               object:self];
                           }];

  // Set this coordinator as the popup menu delegate; this is used to
  // dismiss the popup in response to popup menu requests for dismissal.
  [toolsPopupController_ setDelegate:self];

  [self updateConfiguration];
}

- (void)updateConfiguration {
  // The ToolsMenuConfiguration provided to the ToolsPopupController is not
  // reloadable, but the ToolsPopupController has properties that can be
  // configured dynamically.
  if ([self.configurationProvider
          respondsToSelector:@selector
          (shouldHighlightBookmarkButtonForToolsMenuCoordinator:)])
    [toolsPopupController_
        setIsCurrentPageBookmarked:
            [self.configurationProvider
                shouldHighlightBookmarkButtonForToolsMenuCoordinator:self]];
  if ([self.configurationProvider respondsToSelector:@selector
                                  (shouldShowFindBarForToolsMenuCoordinator:)])
    [toolsPopupController_
        setCanShowFindBar:[self.configurationProvider
                              shouldShowFindBarForToolsMenuCoordinator:self]];
  if ([self.configurationProvider
          respondsToSelector:@selector
          (shouldShowShareMenuForToolsMenuCoordinator:)])
    [toolsPopupController_
        setCanShowShareMenu:
            [self.configurationProvider
                shouldShowShareMenuForToolsMenuCoordinator:self]];
  if ([self.configurationProvider
          respondsToSelector:@selector(tabIsLoadingForToolsMenuCoordinator:)])
    [toolsPopupController_
        setIsTabLoading:[self.configurationProvider
                            tabIsLoadingForToolsMenuCoordinator:self]];
}

- (void)dismissToolsMenu {
  if (![self isShowingToolsMenu])
    return;

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kToolsMenuWillHideNotification
                    object:self];

  ToolsPopupController* tempTPC = toolsPopupController_;
  [toolsPopupController_ containerView].userInteractionEnabled = NO;
  [toolsPopupController_ dismissAnimatedWithCompletion:^{
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kToolsMenuDidHideNotification
                      object:self];

    // Keep the popup controller alive until the animation ends.
    [tempTPC self];
  }];

  toolsPopupController_ = nil;
}

- (BOOL)isShowingToolsMenu {
  return !!toolsPopupController_;
}

#pragma mark - PopupMenuDelegate

- (void)dismissPopupMenu:(PopupMenuController*)controller {
  if ([controller isKindOfClass:[ToolsPopupController class]] &&
      (ToolsPopupController*)controller == toolsPopupController_)
    [self dismissToolsMenu];
}

#pragma mark - Chrome Coordinator interface

- (void)start {
  [self showToolsMenu];
}

- (void)stop {
  [self dismissToolsMenu];
}

@end
