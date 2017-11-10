// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_TOOLBAR_PUBLIC_INTERFACE_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_TOOLBAR_PUBLIC_INTERFACE_H_

@protocol BVCToolbarControllerPublicInterface;
@class ToolsPopupController;
@class ToolsMenuConfiguration;
@class Tab;
@protocol WebToolbarDelegate;

#import <Foundation/Foundation.h>

// WebToolbarController public interface for BVC.
@protocol BVCWebToolbarPublicInterface<BVCToolbarControllerPublicInterface>
// Called when the browser state this object was initialized with is being
// destroyed.
- (void)browserStateDestroyed;
// Update the visibility of the back/forward buttons, omnibox, etc.
- (void)updateToolbarState;
// Briefly animate the progress bar when a pre-rendered tab is displayed.
- (void)showPrerenderingAnimation;
// Called when the current page starts loading.
- (void)currentPageLoadStarted;
// Returns visible omnibox frame in WebToolbarController's view coordinate
// system.
- (CGRect)visibleOmniboxFrame;
// Returns whether omnibox is a first responder.
- (BOOL)isOmniboxFirstResponder;
// Returns whether the omnibox popup is currently displayed.
- (BOOL)showingOmniboxPopup;
// Called when the current tab changes or is closed.
- (void)selectedTabChanged;
// WebToolbarDelegate delegate.
@property(nonatomic, weak) id<WebToolbarDelegate> delegate;

#pragma mark SideSwipeToolbarInteracting

// Update the visibility of the toolbar before making a side swipe snapshot so
// the toolbar looks appropriate for |tab|. This includes morphing the toolbar
// to look like the new tab page header.
- (void)updateToolbarForSideSwipeSnapshot:(Tab*)tab;
// Remove any formatting added by -updateToolbarForSideSwipeSnapshot.
- (void)resetToolbarAfterSideSwipeSnapshot;
@end

#pragma mark - ToolbarController

// ToolbarController public interface for BVC.
@protocol BVCToolbarControllerPublicInterface<NSObject>
// Sets whether the share button is enabled or not.
- (void)setShareButtonEnabled:(BOOL)enabled;
// Triggers an animation on the tools menu button to draw the user's
// attention.
- (void)triggerToolsMenuButtonAnimation;
// Adjusts the height of the Toolbar to match the current UI layout.
- (void)adjustToolbarHeight;
// Sets the background to a particular alpha value. Intended for use by
// subcleasses that need to set the opacity of the entire toolbar.
- (void)setBackgroundAlpha:(CGFloat)alpha;
// Updates the tab stack button (if there is one) based on the given tab
// count. If |tabCount| > |kStackButtonMaxTabCount|, an easter egg is shown
// instead of the actual number of tabs.
- (void)setTabCount:(NSInteger)tabCount;
// Activates constraints to simulate a safe area with |fakeSafeAreaInsets|
// insets. The insets will be used as leading/trailing wrt RTL. Those
// constraints have a higher priority than the one used to respect the safe
// area. They need to be deactivated for the toolbar to respect the safe area
// again. The fake safe area can be bigger or smaller than the real safe area.
- (void)activateFakeSafeAreaInsets:(UIEdgeInsets)fakeSafeAreaInsets;
// Deactivates the constraints used to create a fake safe area.
- (void)deactivateFakeSafeAreaInsets;
// The view for the toolbar background image. This is a subview of |view| to
// allow clients to alter the transparency of the background image without
// affecting the other components of the toolbar.
@property(nonatomic, readonly, strong) UIImageView* backgroundView;

// Following methods will be removed shortly by CL 741466.
#pragma mark - ToolsMenu
- (void)showToolsMenuPopupWithConfiguration:
    (ToolsMenuConfiguration*)configuration;
- (void)dismissToolsMenuPopup;
@property(nonatomic, readonly, strong)
    ToolsPopupController* toolsPopupController;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_PUBLIC_TOOLBAR_PUBLIC_INTERFACE_H_
