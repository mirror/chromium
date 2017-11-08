// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/toolbar_coordinator.h"

#import "ios/chrome/browser/ui/toolbar/toolbar_utils.h"
#import "ios/chrome/browser/ui/toolbar/web_toolbar_controller.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LegacyToolbarCoordinator () {
  // Coordinator for the tools menu UI.
  ToolsMenuCoordinator* _toolsMenuCoordinator;
}
@end

@implementation LegacyToolbarCoordinator
@synthesize tabModel = _tabModel;
@synthesize webToolbarController = _webToolbarController;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
            toolsMenuConfigurationProvider:
                (id<ToolsMenuCoordinatorConfigurationProvider>)
                    configurationProvider
                                dispatcher:(CommandDispatcher*)dispatcher {
  if (self = [super initWithBaseViewController:viewController]) {
    _toolsMenuCoordinator = [[ToolsMenuCoordinator alloc]
        initWithBaseViewController:viewController];
    _toolsMenuCoordinator.dispatcher = dispatcher;
    _toolsMenuCoordinator.configurationProvider = configurationProvider;

    NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
    [defaultCenter addObserver:self
                      selector:@selector(toolsMenuWillShowNotification:)
                          name:kToolsMenuWillShowNotification
                        object:_toolsMenuCoordinator];
    [defaultCenter addObserver:self
                      selector:@selector(toolsMenuWillHideNotification:)
                          name:kToolsMenuWillHideNotification
                        object:_toolsMenuCoordinator];
  }
  return self;
}

- (ToolbarController*)toolbarController {
  return self.webToolbarController;
}

- (id<VoiceSearchControllerDelegate>)voiceSearchDelegate {
  return self.webToolbarController;
}

- (id<ActivityServicePositioner>)activityServicePositioner {
  return self.webToolbarController;
}

- (id<TabHistoryPositioner>)tabHistoryPositioner {
  return self.webToolbarController;
}

- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater {
  return self.webToolbarController;
}

- (id<QRScannerResultLoading>)QRScannerResultLoader {
  return self.webToolbarController;
}

- (void)setWebToolbar:(WebToolbarController*)webToolbarController {
  self.webToolbarController = webToolbarController;
  // ToolbarController needs to know about whether the tools menu is presented
  // or not, and does so by storing a reference to the coordinator to query.
  self.toolbarController.toolsMenuStateProvider = _toolsMenuCoordinator;
  _toolsMenuCoordinator.presentationProvider = webToolbarController;
}

- (void)setToolbarDelegate:(id<WebToolbarDelegate>)delegate {
  self.webToolbarController.delegate = delegate;
}

- (void)adjustToolbarHeight {
  self.webToolbarController.heightConstraint.constant =
      ToolbarHeightWithTopOfScreenOffset(
          [self.webToolbarController statusBarOffset]);
  self.webToolbarController.heightConstraint.active = YES;
}

#pragma mark - WebToolbarController interface

- (void)selectedTabChanged {
  [self.webToolbarController selectedTabChanged];
}

- (void)setTabCount:(NSInteger)tabCount {
  [self.webToolbarController setTabCount:tabCount];
}

- (void)browserStateDestroyed {
  [self.webToolbarController setBackgroundAlpha:1.0];
  [self.webToolbarController browserStateDestroyed];
}

- (void)updateToolbarState {
  [self.webToolbarController updateToolbarState];

  // Also update the loading state for the tools menu (that is really an
  // extension of the toolbar on the iPhone).
  [_toolsMenuCoordinator updateConfiguration];
}

- (void)setShareButtonEnabled:(BOOL)enabled {
  [self.webToolbarController setShareButtonEnabled:enabled];
}

- (void)showPrerenderingAnimation {
  [self.webToolbarController showPrerenderingAnimation];
}

- (BOOL)isOmniboxFirstResponder {
  return [self.webToolbarController isOmniboxFirstResponder];
}

- (BOOL)showingOmniboxPopup {
  return [self.webToolbarController showingOmniboxPopup];
}

- (void)currentPageLoadStarted {
  [self.webToolbarController currentPageLoadStarted];
}

- (CGRect)visibleOmniboxFrame {
  return [self.webToolbarController visibleOmniboxFrame];
}

- (void)triggerToolsMenuButtonAnimation {
  [self.webToolbarController triggerToolsMenuButtonAnimation];
}

- (UIView*)view {
  return [self.webToolbarController view];
}

#pragma mark - OmniboxFocuser

- (void)focusOmnibox {
  [self.webToolbarController focusOmnibox];
}

- (void)cancelOmniboxEdit {
  [self.webToolbarController cancelOmniboxEdit];
}

- (void)focusFakebox {
  [self.webToolbarController focusFakebox];
}

- (void)onFakeboxBlur {
  [self.webToolbarController onFakeboxBlur];
}

- (void)onFakeboxAnimationComplete {
  [self.webToolbarController onFakeboxAnimationComplete];
}

#pragma mark - IncognitoViewControllerDelegate

- (void)setToolbarBackgroundAlpha:(CGFloat)alpha {
  [self.webToolbarController setBackgroundAlpha:alpha];
}

#pragma mark - BubbleViewAnchorPointProvider methods.

- (CGPoint)anchorPointForTabSwitcherButton:(BubbleArrowDirection)direction {
  return [self.webToolbarController anchorPointForTabSwitcherButton:direction];
}

- (CGPoint)anchorPointForToolsMenuButton:(BubbleArrowDirection)direction {
  return [self.webToolbarController anchorPointForToolsMenuButton:direction];
}

#pragma mark - Tools Menu

- (BOOL)isShowingToolsMenu {
  return [_toolsMenuCoordinator isShowingToolsMenu];
}

- (void)toolsMenuWillShowNotification:(NSNotification*)note {
  [self.webToolbarController setToolsMenuIsVisibleForToolsMenuButton:YES];
}

- (void)toolsMenuWillHideNotification:(NSNotification*)note {
  [self.webToolbarController setToolsMenuIsVisibleForToolsMenuButton:NO];
}

@end
