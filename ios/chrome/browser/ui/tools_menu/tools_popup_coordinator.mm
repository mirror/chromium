// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_popup_coordinator.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/ui/commands/tools_popup_commands.h"
#import "ios/chrome/browser/ui/tools_menu/tools_popup_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kToolsPopupMenuWillShowNotification =
    @"kToolsPopupMenuWillShowNotification";
NSString* const kToolsPopupMenuWillHideNotification =
    @"kToolsPopupMenuWillHideNotification";
NSString* const kToolsPopupMenuDidShowNotification =
    @"kToolsPopupMenuDidShowNotification";
NSString* const kToolsPopupMenuDidHideNotification =
    @"kToolsPopupMenuDidHideNotification";

@interface ToolsPopupCoordinator ()<ToolsPopupCommands, PopupMenuDelegate> {
  // The following is nil if not visible.
  ToolsPopupController* toolsPopupController_;
}
@end

@implementation ToolsPopupCoordinator
@synthesize presentationProvider = presentationProvider_;
@synthesize configurationProvider = configurationProvider_;
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

- (void)showToolsPopup {
  ToolsMenuConfiguration* configuration = [self.configurationProvider
      menuConfigurationForToolsPopupCoordinator:self];
  [self showToolsMenuPopupWithConfiguration:configuration
                                 dispatcher:self.dispatcher];
}

- (void)dismissToolsPopup {
  [self dismissToolsMenuPopup];
}

- (void)showToolsMenuPopupWithConfiguration:
            (ToolsMenuConfiguration*)configuration
                                 dispatcher:(CommandDispatcher*)dispatcher {
  // Because an animation hides and shows the tools popup menu it is possible to
  // tap the tools button multiple times before the tools menu is shown. Ignore
  // repeated taps between animations.
  if ([self isShowingToolsMenuPopup])
    return;

  base::RecordAction(base::UserMetricsAction("ShowAppMenu"));

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kToolsPopupMenuWillShowNotification
                    object:self];

  // This allows for the presenting button to be shown above the presented menu.
  [configuration
      setToolsMenuButton:[self.presentationProvider
                             presentingButtonForPopupCoordinator:self]];

  toolsPopupController_ = [[ToolsPopupController alloc]
      initAndPresentWithConfiguration:configuration
                           dispatcher:(id<ApplicationCommands, BrowserCommands>)
                                          dispatcher
                           completion:^{
                             [[NSNotificationCenter defaultCenter]
                                 postNotificationName:
                                     kToolsPopupMenuDidShowNotification
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
          (shouldHighlightBookmarkButtonForToolsPopupCoordinator:)])
    [toolsPopupController_
        setIsCurrentPageBookmarked:
            [self.configurationProvider
                shouldHighlightBookmarkButtonForToolsPopupCoordinator:self]];
  if ([self.configurationProvider respondsToSelector:@selector
                                  (shouldShowFindBarForToolsPopupCoordinator:)])
    [toolsPopupController_
        setCanShowFindBar:[self.configurationProvider
                              shouldShowFindBarForToolsPopupCoordinator:self]];
  if ([self.configurationProvider
          respondsToSelector:@selector
          (shouldShowShareMenuForToolsPopupCoordinator:)])
    [toolsPopupController_
        setCanShowShareMenu:
            [self.configurationProvider
                shouldShowShareMenuForToolsPopupCoordinator:self]];
  if ([self.configurationProvider
          respondsToSelector:@selector(tabIsLoadingForToolsPopupCoordinator:)])
    [toolsPopupController_
        setIsTabLoading:[self.configurationProvider
                            tabIsLoadingForToolsPopupCoordinator:self]];
}

- (void)dismissToolsMenuPopup {
  if (![self isShowingToolsMenuPopup])
    return;

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kToolsPopupMenuWillHideNotification
                    object:self];

  ToolsPopupController* tempTPC = toolsPopupController_;
  [toolsPopupController_ containerView].userInteractionEnabled = NO;
  [toolsPopupController_ dismissAnimatedWithCompletion:^{
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kToolsPopupMenuDidHideNotification
                      object:self];

    // Keep the popup controller alive until the animation ends.
    [tempTPC self];
  }];

  toolsPopupController_ = nil;
}

- (BOOL)isShowingToolsMenuPopup {
  return !!toolsPopupController_;
}

#pragma mark - PopupMenuDelegate

- (void)dismissPopupMenu:(PopupMenuController*)controller {
  if ([controller isKindOfClass:[ToolsPopupController class]] &&
      (ToolsPopupController*)controller == toolsPopupController_)
    [self dismissToolsMenuPopup];
}

@end
