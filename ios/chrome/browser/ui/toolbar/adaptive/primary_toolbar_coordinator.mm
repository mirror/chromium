// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_coordinator.h"

#import <CoreLocation/CoreLocation.h>

#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "components/google/core/browser/google_util.h"
#import "ios/chrome/browser/ui/location_bar/location_bar_coordinator.h"
#include "ios/chrome/browser/ui/omnibox/location_bar_controller_impl.h"
#include "ios/chrome/browser/ui/omnibox/location_bar_delegate.h"
#import "ios/chrome/browser/ui/omnibox/omnibox_popup_positioner.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_coordinator+subclassing.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view_controller.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_coordinator_delegate.h"
#import "ios/chrome/browser/ui/toolbar/public/web_toolbar_controller_constants.h"
#include "ios/chrome/browser/ui/toolbar/toolbar_model_ios.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#include "ios/web/public/referrer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PrimaryToolbarCoordinator ()<OmniboxPopupPositioner>

// Redefined as PrimaryToolbarViewController.
@property(nonatomic, strong) PrimaryToolbarViewController* viewController;
// The coordinator for the location bar in the toolbar.
@property(nonatomic, strong) LocationBarCoordinator* locationBarCoordinator;

@end

@implementation PrimaryToolbarCoordinator

@dynamic viewController;
@synthesize locationBarCoordinator = _locationBarCoordinator;
@synthesize delegate = _delegate;
@synthesize URLLoader = _URLLoader;

#pragma mark - ChromeCoordinator

- (void)start {
  self.viewController = [[PrimaryToolbarViewController alloc] init];
  self.viewController.buttonFactory = [self buttonFactoryWithType:PRIMARY];

  [self setUpLocationBar];
  self.viewController.locationBarView =
      self.locationBarCoordinator.locationBarView;
  [super start];
}

#pragma mark - PrimaryToolbarCoordinator

- (id<VoiceSearchControllerDelegate>)voiceSearchDelegate {
  // TODO(crbug.com/799446): This code should be moved to the location bar.
  return nil;
}

- (id<QRScannerResultLoading>)QRScannerResultLoader {
  // TODO(crbug.com/799446): This code should be moved to the location bar.
  return nil;
}

- (id<TabHistoryUIUpdater>)tabHistoryUIUpdater {
  // TODO(crbug.com/803373): Implement that.
  return nil;
}

- (id<ActivityServicePositioner>)activityServicePositioner {
  // TODO(crbug.com/803376): Implement that.
  return nil;
}

- (id<OmniboxFocuser>)omniboxFocuser {
  return self.locationBarCoordinator;
}

- (void)showPrerenderingAnimation {
  [self.viewController showPrerenderingAnimation];
}

- (BOOL)isOmniboxFirstResponder {
  return
      [self.locationBarCoordinator.locationBarView.textField isFirstResponder];
}

- (BOOL)showingOmniboxPopup {
  return [self.locationBarCoordinator showingOmniboxPopup];
}

- (void)transitionToLocationBarFocusedState:(BOOL)focused {
  // TODO(crbug.com/801082): implement this.
}

#pragma mark - ToolbarCoordinating

- (void)updateToolbarState {
  // TODO(crbug.com/803383): This should be done inside the location bar.
  [self.locationBarCoordinator updateOmniboxState];
  [super updateToolbarState];
}

#pragma mark - FakeboxFocuser

- (void)focusFakebox {
  // TODO(crbug.com/803372): Implement that.
}

- (void)onFakeboxBlur {
  // TODO(crbug.com/803372): Implement that.
}

- (void)onFakeboxAnimationComplete {
  // TODO(crbug.com/803372): Implement that.
}

// TODO(crbug.com/786940): This protocol should move to the ViewController
// owning the Toolbar. This can wait until the omnibox and toolbar refactoring
// is more advanced.
#pragma mark OmniboxPopupPositioner methods.

- (UIView*)popupAnchorView {
  return self.viewController.view;
}

#pragma mark - SideSwipeToolbarInteracting

- (UIView*)toolbarView {
  return self.viewController.view;
}

- (BOOL)canBeginToolbarSwipe {
  return ![self isOmniboxFirstResponder] && ![self showingOmniboxPopup];
}

- (UIImage*)toolbarSideSwipeSnapshotForTab:(Tab*)tab {
  // TODO(crbug.com/803371): Implement that.
  return nil;
}

#pragma mark - Private

// Sets the location bar up.
- (void)setUpLocationBar {
  self.locationBarCoordinator = [[LocationBarCoordinator alloc] init];
  self.locationBarCoordinator.browserState = self.browserState;
  self.locationBarCoordinator.dispatcher = self.dispatcher;
  self.locationBarCoordinator.URLLoader = self.URLLoader;
  self.locationBarCoordinator.delegate = self.delegate;
  self.locationBarCoordinator.webStateList = self.webStateList;
  self.locationBarCoordinator.popupPositioner = self;
  [self.locationBarCoordinator start];
}

@end
