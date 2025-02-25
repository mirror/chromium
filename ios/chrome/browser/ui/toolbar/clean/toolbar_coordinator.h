// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/public/fakebox_focuser.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_presentation_provider.h"

@protocol ActivityServicePositioner;
@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol OmniboxFocuser;
@class ToolbarButtonUpdater;
@protocol ToolbarCoordinatorDelegate;
@protocol UrlLoader;
@protocol VoiceSearchControllerDelegate;
@protocol QRScannerResultLoading;
class WebStateList;
namespace ios {
class ChromeBrowserState;
}
namespace web {
class WebState;
}

// Coordinator to run a toolbar -- a UI element housing controls.
@interface ToolbarCoordinator
    : NSObject<FakeboxFocuser, ToolsMenuPresentationProvider>

// Weak reference to ChromeBrowserState;
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
// The dispatcher for this view controller.
@property(nonatomic, weak)
    id<ApplicationCommands, BrowserCommands, OmniboxFocuser>
        dispatcher;
// The web state list this ToolbarCoordinator is handling.
@property(nonatomic, assign) WebStateList* webStateList;
// Delegate for this coordinator. Only used for plumbing to Location Bar
// coordinator.
// TODO(crbug.com/799446): Change this.
@property(nonatomic, weak) id<ToolbarCoordinatorDelegate> delegate;
// URL loader for the toolbar.
// TODO(crbug.com/799446): Remove this.
@property(nonatomic, weak) id<UrlLoader> URLLoader;
// UIViewController managed by this coordinator.
@property(nonatomic, strong, readonly) UIViewController* viewController;
// Button updater for the toolbar.
@property(nonatomic, strong) ToolbarButtonUpdater* buttonUpdater;

// Returns the ActivityServicePositioner for this toolbar.
- (id<ActivityServicePositioner>)activityServicePositioner;

// Returns the OmniboxFocuser for this toolbar.
- (id<OmniboxFocuser>)omniboxFocuser;
// Returns the VoiceSearchControllerDelegate for this toolbar.
- (id<VoiceSearchControllerDelegate>)voiceSearchControllerDelegate;
// Returns the QRScannerResultLoading for this toolbar.
- (id<QRScannerResultLoading>)QRScannerResultLoader;

// Start this coordinator.
- (void)start;
// Stop this coordinator.
- (void)stop;

// Coordinates the location bar focusing/defocusing. For example, initiates
// transition to the expanded location bar state of the view controller.
- (void)transitionToLocationBarFocusedState:(BOOL)focused;

// Updates the toolbar so it is in a state where a snapshot for |webState| can
// be taken.
- (void)updateToolbarForSideSwipeSnapshot:(web::WebState*)webState;
// Resets the toolbar after taking a side swipe snapshot. After calling this
// method the toolbar is adapted to the current webState.
- (void)resetToolbarAfterSideSwipeSnapshot;
// Sets the ToolsMenu visibility depending if the tools menu |isVisible|.
- (void)setToolsMenuIsVisibleForToolsMenuButton:(BOOL)isVisible;
// Triggers the animation of the tools menu button.
- (void)triggerToolsMenuButtonAnimation;
// Sets the background color of the Toolbar to the one of the Incognito NTP,
// with an |alpha|.
- (void)setBackgroundToIncognitoNTPColorWithAlpha:(CGFloat)alpha;
// Briefly animate the progress bar when a pre-rendered tab is displayed.
- (void)showPrerenderingAnimation;
// Returns whether omnibox is a first responder.
- (BOOL)isOmniboxFirstResponder;
// Returns whether the omnibox popup is currently displayed.
- (BOOL)showingOmniboxPopup;
// Activates constraints to simulate a safe area with |fakeSafeAreaInsets|
// insets. The insets will be used as leading/trailing wrt RTL. Those
// constraints have a higher priority than the one used to respect the safe
// area. They need to be deactivated for the toolbar to respect the safe area
// again. The fake safe area can be bigger or smaller than the real safe area.
- (void)activateFakeSafeAreaInsets:(UIEdgeInsets)fakeSafeAreaInsets;
// Deactivates the constraints used to create a fake safe area.
- (void)deactivateFakeSafeAreaInsets;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_CLEAN_TOOLBAR_COORDINATOR_H_
