// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/clean/toolbar_coordinator.h"

#import <CoreLocation/CoreLocation.h>

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/google/core/browser/google_util.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/ui/omnibox/location_bar_controller.h"
#include "ios/chrome/browser/ui/omnibox/location_bar_controller_impl.h"
#include "ios/chrome/browser/ui/omnibox/location_bar_delegate.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_coordinator_audience.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_mediator.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_style.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_view_controller.h"
#import "ios/chrome/browser/ui/toolbar/public/web_toolbar_controller_constants.h"
#import "ios/chrome/browser/ui/url_loader.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolbarCoordinator ()<LocationBarDelegate> {
  std::unique_ptr<LocationBarControllerImpl> _locationBar;
}

// The View Controller managed by this coordinator.
@property(nonatomic, strong) ToolbarViewController* viewController;
// The mediator owned by this coordinator.
@property(nonatomic, strong) ToolbarMediator* mediator;
// LocationBarView containing the omnibox. At some point, this property and the
// |_locationBar| should become a LocationBarCoordinator.
@property(nonatomic, strong) LocationBarView* locationBarView;

@end

@implementation ToolbarCoordinator
@synthesize audience = _audience;
@synthesize chromeBrowserState = _chromeBrowserState;
@synthesize dispatcher = _dispatcher;
@synthesize locationBarView = _locationBarView;
@synthesize mediator = _mediator;
@synthesize URLLoader = _URLLoader;
@synthesize viewController = _viewController;
@synthesize webStateList = _webStateList;

- (instancetype)init {
  if ((self = [super init])) {
    _mediator = [[ToolbarMediator alloc] init];
  }
  return self;
}

#pragma mark - BrowserCoordinator

- (void)start {
  self.locationBarView = nil;
  _locationBar = base::MakeUnique<LocationBarControllerImpl>(
      self.locationBarView, self.chromeBrowserState, self, self.dispatcher);

  ToolbarStyle style =
      self.chromeBrowserState->IsOffTheRecord() ? INCOGNITO : NORMAL;
  ToolbarButtonFactory* factory =
      [[ToolbarButtonFactory alloc] initWithStyle:style];

  self.viewController =
      [[ToolbarViewController alloc] initWithDispatcher:self.dispatcher
                                          buttonFactory:factory];

  self.mediator.consumer = self.viewController;
  self.mediator.webStateList = self.webStateList;
}

- (void)stop {
  [self.mediator disconnect];
}

#pragma mark - LocationBarDelegate

- (void)loadGURLFromLocationBar:(const GURL&)url
                     transition:(ui::PageTransition)transition {
  if (url.SchemeIs(url::kJavaScriptScheme)) {
    // Evaluate the URL as JavaScript if its scheme is JavaScript.
    NSString* jsToEval = [base::SysUTF8ToNSString(url.GetContent())
        stringByRemovingPercentEncoding];
    [self.URLLoader loadJavaScriptFromLocationBar:jsToEval];
  } else {
    // When opening a URL, force the omnibox to resign first responder.  This
    // will also close the popup.

    // TODO(crbug.com/785244): Is it ok to call |cancelOmniboxEdit| after
    // |loadURL|?  It doesn't seem to be causing major problems.  If we call
    // cancel before load, then any prerendered pages get destroyed before the
    // call to load.
    [self.URLLoader loadURL:url
                   referrer:web::Referrer()
                 transition:transition
          rendererInitiated:NO];

    if (google_util::IsGoogleSearchUrl(url)) {
      UMA_HISTOGRAM_ENUMERATION(
          kOmniboxQueryLocationAuthorizationStatusHistogram,
          [CLLocationManager authorizationStatus],
          kLocationAuthorizationStatusCount);
    }
  }
  [self cancelOmniboxEdit];
}

- (void)locationBarHasBecomeFirstResponder {
  [self.audience locationBarDidBecomeFirstResponder];
  if (@available(iOS 10, *)) {
    [self.viewController expandOmnibox];
  }
}

- (void)locationBarHasResignedFirstResponder {
  [self.audience locationBarDidResignFirstResponder];
  if (@available(iOS 10, *)) {
    [self.viewController contractOmnibox];
  }
}

- (void)locationBarBeganEdit {
  [self.audience locationBarBeganEdit];
}

- (web::WebState*)getWebState {
  return self.webStateList->GetActiveWebState();
}

- (ToolbarModel*)toolbarModel {
  return nullptr;
}

// TODO(crbug.com/78911): implement this protocol.
#pragma mark - OmniboxFocuser

- (void)cancelOmniboxEdit {
  // TODO(crbug.com/784911): Implement this.
}

@end
