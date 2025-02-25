// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/location_bar/location_bar_url_loader.h"
#import "ios/chrome/browser/ui/omnibox/location_bar_delegate.h"
#include "ios/chrome/browser/ui/qr_scanner/requirements/qr_scanner_result_loading.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#include "ios/public/provider/chrome/browser/voice/voice_search_controller_delegate.h"

namespace ios {
class ChromeBrowserState;
}
class WebStateList;
@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol OmniboxPopupPositioner;
@protocol ToolbarCoordinatorDelegate;
@protocol UrlLoader;

@interface LocationBarCoordinator : NSObject<LocationBarURLLoader,
                                             OmniboxFocuser,
                                             VoiceSearchControllerDelegate,
                                             QRScannerResultLoading>

// View containing the omnibox.
@property(nonatomic, strong, readonly) UIView* view;
// Weak reference to ChromeBrowserState;
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
// The dispatcher for this view controller.
@property(nonatomic, weak) id<ApplicationCommands, BrowserCommands> dispatcher;
// URL loader for the location bar.
@property(nonatomic, weak) id<UrlLoader> URLLoader;
// Delegate for this coordinator.
// TODO(crbug.com/799446): Change this.
@property(nonatomic, weak) id<ToolbarCoordinatorDelegate> delegate;
// The web state list this ToolbarCoordinator is handling.
@property(nonatomic, assign) WebStateList* webStateList;

@property(nonatomic, weak) id<OmniboxPopupPositioner> popupPositioner;

// Start this coordinator.
- (void)start;
// Stop this coordinator.
- (void)stop;

// Indicates whether the popup has results to show or not.
- (BOOL)omniboxPopupHasAutocompleteResults;

// Indicates if the omnibox currently displays a popup with suggestions.
- (BOOL)showingOmniboxPopup;

// Focuses the omnibox and sets the caret visibility as if it was called from
// the fakebox on NTP.
- (void)focusOmniboxFromFakebox;

// Indicates when the omnibox is the first responder.
- (BOOL)isOmniboxFirstResponder;

// Perform animations for expanding the omnibox. This animation can be seen on
// an iPhone when the omnibox is focused. It involves sliding the leading button
// out and fading its alpha.
// The trailing button is faded-in in the |completionAnimator| animations.
- (void)addExpandOmniboxAnimations:(UIViewPropertyAnimator*)animator
                completionAnimator:(UIViewPropertyAnimator*)completionAnimator;

// Perform animations for expanding the omnibox. This animation can be seen on
// an iPhone when the omnibox is defocused. It involves sliding the leading
// button in and fading its alpha.
- (void)addContractOmniboxAnimations:(UIViewPropertyAnimator*)animator;

// Updates omnibox state, including the displayed text and the cursor position.
- (void)updateOmniboxState;

@end

#endif  // IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_COORDINATOR_H_
