// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_coordinator.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/chrome/browser/ui/commands/tools_menu_commands.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_configuration_provider.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_provider.h"
#import "ios/chrome/browser/ui/tools_menu/tools_popup_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolsMenuCoordinator ()<ToolsMenuCommands, PopupMenuDelegate> {
  // The following is nil if not visible.
  ToolsPopupController* _toolsPopupController;
}
@end

@implementation ToolsMenuCoordinator
@synthesize configurationProvider = _configurationProvider;
@synthesize presentationProvider = _presentationProvider;
@synthesize dispatcher = _dispatcher;
@synthesize toolsMenuConfiguration = _toolsMenuConfiguration;
@synthesize shouldShowFindBar = _shouldShowFindBar;
@synthesize shouldShowShareMenu = _shouldShowShareMenu;
@synthesize shouldHighlightBookmarkButton = _shouldHighlightBookmarkButton;
@synthesize tabIsLoading = _tabIsLoading;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController {
  if (self = [super initWithBaseViewController:viewController]) {
    NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self
                      selector:@selector(applicationDidEnterBackground:)
                          name:UIApplicationDidEnterBackgroundNotification
                        object:nil];
  }
  return self;
}

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
                               forProtocol:@protocol(ToolsMenuCommands)];
    }
    _dispatcher = dispatcher;
  }
}

- (BOOL)isShowingToolsMenu {
  return !!_toolsPopupController;
}

#pragma mark - Tools Menu API

- (void)setShouldShowFindBar:(BOOL)shouldShowFindBar {
  _shouldShowFindBar = shouldShowFindBar;
  if (_toolsPopupController) {
    [_toolsPopupController setCanShowFindBar:shouldShowFindBar];
  }
}

- (void)setShouldShowShareMenu:(BOOL)shouldShowShareMenu {
  _shouldShowShareMenu = shouldShowShareMenu;
  if (_toolsPopupController) {
    [_toolsPopupController setCanShowShareMenu:shouldShowShareMenu];
  }
}

- (void)setShouldHighlightBookmarkButton:(BOOL)shouldHighlightBookmarkButton {
  _shouldHighlightBookmarkButton = shouldHighlightBookmarkButton;
  if (_toolsPopupController) {
    [_toolsPopupController
        setIsCurrentPageBookmarked:shouldHighlightBookmarkButton];
  }
}

- (void)setTabIsLoading:(BOOL)tabIsLoading {
  _tabIsLoading = tabIsLoading;
  if (_toolsPopupController) {
    [_toolsPopupController setIsTabLoading:tabIsLoading];
  }
}

#pragma mark - ToolsMenuCommands

- (void)showToolsMenu {
  // Because an animation hides and shows the tools popup menu it is possible to
  // tap the tools button multiple times before the tools menu is shown. Ignore
  // repeated taps between animations.
  if ([self isShowingToolsMenu])
    return;

  if ([self.configurationProvider
          respondsToSelector:@selector
          (prepareForToolsMenuPresentationByCoordinator:)]) {
    [self.configurationProvider
        prepareForToolsMenuPresentationByCoordinator:self];
  }
  base::RecordAction(base::UserMetricsAction("ShowAppMenu"));

  self.toolsMenuConfiguration.toolsMenuButton =
      [self.presentationProvider presentingButtonForToolsMenuCoordinator:self];

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kToolsMenuWillShowNotification
                    object:nil];

  _toolsPopupController = [[ToolsPopupController alloc]
      initAndPresentWithConfiguration:self.toolsMenuConfiguration
                           dispatcher:(id<ApplicationCommands, BrowserCommands>)
                                          self.dispatcher
                           completion:^{
                             [[NSNotificationCenter defaultCenter]
                                 postNotificationName:
                                     kToolsMenuDidShowNotification
                                               object:nil];
                           }];

  [_toolsPopupController
      setIsCurrentPageBookmarked:self.shouldHighlightBookmarkButton];
  [_toolsPopupController setIsTabLoading:self.tabIsLoading];
  [_toolsPopupController setCanShowShareMenu:self.shouldShowShareMenu];
  [_toolsPopupController setCanShowFindBar:self.shouldShowFindBar];

  // Set this coordinator as the popup menu delegate; this is used to
  // dismiss the popup in response to popup menu requests for dismissal.
  [_toolsPopupController setDelegate:self];
}

- (void)dismissToolsMenu {
  if (![self isShowingToolsMenu])
    return;

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kToolsMenuWillHideNotification
                    object:nil];

  ToolsPopupController* tempTPC = _toolsPopupController;
  [_toolsPopupController containerView].userInteractionEnabled = NO;
  [_toolsPopupController dismissAnimatedWithCompletion:^{
    [[NSNotificationCenter defaultCenter]
        postNotificationName:kToolsMenuDidHideNotification
                      object:nil];

    // Keep the popup controller alive until the animation ends.
    [tempTPC self];
  }];

  _toolsPopupController = nil;
}

- (void)applicationDidEnterBackground:(NSNotification*)note {
  [self dismissToolsMenu];
}

#pragma mark - PopupMenuDelegate

- (void)dismissPopupMenu:(PopupMenuController*)controller {
  if ([controller isKindOfClass:[ToolsPopupController class]] &&
      (ToolsPopupController*)controller == _toolsPopupController)
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
