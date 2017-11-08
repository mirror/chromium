// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/ui/bubble/bubble_view_anchor_point_provider.h"
#import "ios/chrome/browser/ui/ntp/incognito_view_controller_delegate.h"
#import "ios/chrome/browser/ui/toolbar/omnibox_focuser.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_coordinator_configuration_provider.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_coordinator_presentation_provider.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_coordinator_presentation_state.h"

@protocol ActivityServicePositioner;
@protocol QRScannerResultLoading;
@protocol TabHistoryPositioner;
@protocol TabHistoryUIUpdater;
@protocol VoiceSearchControllerDelegate;
@protocol WebToolbarDelegate;

@class CommandDispatcher;
@class TabModel;
@class ToolbarController;
@class WebToolbarController;

@interface LegacyToolbarCoordinator
    : ChromeCoordinator<BubbleViewAnchorPointProvider,
                        IncognitoViewControllerDelegate,
                        OmniboxFocuser,
                        ToolsMenuCoordinatorPresentationStateProtocol>

@property(nonatomic, weak) TabModel* tabModel;
@property(nonatomic, readonly, strong) UIView* view;
// TODO(crbug.com/778226): Remove this property.
@property(nonatomic, strong) WebToolbarController* webToolbarController;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
            toolsMenuConfigurationProvider:
                (id<ToolsMenuCoordinatorConfigurationProvider>)
                    configurationProvider
                                dispatcher:(CommandDispatcher*)dispatcher;

// Returns the different protocols and superclass now implemented by the
// WebToolbarController to avoid using the toolbar directly.
- (ToolbarController*)toolbarController;
- (id<VoiceSearchControllerDelegate>)voiceSearchDelegate;
- (id<ActivityServicePositioner>)activityServicePositioner;
- (id<TabHistoryPositioner>)tabHistoryPositioner;
- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater;
- (id<QRScannerResultLoading>)QRScannerResultLoader;

// Sets the WebToolbarController for this coordinator.
- (void)setWebToolbar:(WebToolbarController*)webToolbarController;

// Sets the delegate for the toolbar.
- (void)setToolbarDelegate:(id<WebToolbarDelegate>)delegate;

// Sets the height of the toolbar to be the .
- (void)adjustToolbarHeight;

// TabModel callbacks.
- (void)selectedTabChanged;
- (void)setTabCount:(NSInteger)tabCount;

// WebToolbarController public interface.
- (void)browserStateDestroyed;
- (void)updateToolbarState;
- (void)setShareButtonEnabled:(BOOL)enabled;
- (void)showPrerenderingAnimation;
- (BOOL)isOmniboxFirstResponder;
- (BOOL)showingOmniboxPopup;
- (void)currentPageLoadStarted;
- (CGRect)visibleOmniboxFrame;
- (void)triggerToolsMenuButtonAnimation;

// ToolsMenuCoordinatorPresentationStateProtocol
- (BOOL)isShowingToolsMenu;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_COORDINATOR_H_
