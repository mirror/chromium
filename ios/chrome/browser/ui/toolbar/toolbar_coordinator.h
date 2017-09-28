// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/chrome_coordinator.h"
#import "ios/chrome/browser/ui/toolbar/web_toolbar_controller.h"


@class TabModel;

@interface TToolbarCoordinator : ChromeCoordinator<OmniboxFocuser>

@property(nonatomic, readwrite, weak) TabModel* tabModel;
@property(nonatomic, readonly, strong) UIView* view;

// Called when trait collection changes.
- (void)updateButtonVisibility;

- (void)browserStateDestroyed;

// Can this be driven by the tabmodel instead?
- (void)selectedTabChanged;

- (void)updateToolbarState;

- (void)setShareButtonEnabled:(BOOL)enabled;

- (void)showPrerenderingAnimation;

- (BOOL)isOmniboxFirstResponder;
- (BOOL)showingOmniboxPopup;

- (void)currentPageLoadStarted;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_COORDINATOR_H_
