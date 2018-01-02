// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_ABSTRACT_PRIMARY_TOOLBAR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_ABSTRACT_PRIMARY_TOOLBAR_H_

@protocol QRScannerResultLoading;
@protocol TabHistoryPositioner;
@protocol TabHistoryUIUpdater;
@protocol VoiceSearchControllerDelegate;

@protocol AbstractPrimaryToolbar<OmniboxFocuser, SideSwipeToolbarInteracting>

@property(nonatomic, strong) UIViewController* toolbarViewController;

// Sets the delegate for the toolbar.
- (void)setToolbarDelegate:(id<WebToolbarDelegate>)delegate;

// Returns the different protocols and superclass now implemented by the
// internal ViewController.
- (id<VoiceSearchControllerDelegate>)voiceSearchDelegate;
- (id<QRScannerResultLoading>)QRScannerResultLoader;
- (id<TabHistoryPositioner>)tabHistoryPositioner;
- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater;

- (void)showPrerenderingAnimation;
- (BOOL)isOmniboxFirstResponder;
- (BOOL)showingOmniboxPopup;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_ABSTRACT_PRIMARY_TOOLBAR_H_
