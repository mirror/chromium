// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/omnibox/location_bar_coordinator.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/commands/command_dispatcher.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/omnibox/location_bar_controller_impl.h"
#include "ios/chrome/browser/ui/toolbar/toolbar_model_ios.h"
#import "ios/clean/chrome/browser/ui/omnibox/location_bar_mediator.h"
#import "ios/clean/chrome/browser/ui/omnibox/location_bar_view_controller.h"
#import "ios/clean/chrome/browser/ui/omnibox/popup_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@protocol ABC<NSObject>
- (void)fooBarBaz:(UIViewController*)vc;
@end

@interface LocationBarCoordinator ()<PopupDisplaying>

@property(nonatomic, strong) LocationBarMediator* mediator;
@property(nonatomic, readwrite, strong)
    LocationBarViewController* viewController;
@property(nonatomic, strong) PopupCoordinator* popupCoordinator;
@end

@implementation LocationBarCoordinator {
}

@synthesize mediator = _mediator;
@synthesize viewController = _viewController;
@synthesize popupCoordinator = _popupCoordinator;

- (void)start {
  DCHECK(self.parentCoordinator);
  self.viewController = [[LocationBarViewController alloc] init];

  Browser* browser = self.browser;

  [self.browser->dispatcher()
      startDispatchingToTarget:self
                   forProtocol:@protocol(PopupDisplaying)];

  self.mediator = [[LocationBarMediator alloc]
      initWithWebStateList:&(browser->web_state_list())];
  self.mediator.popupDisplayer = (id)self.browser->dispatcher();

  std::unique_ptr<LocationBarControllerImpl> locationBar =
      base::MakeUnique<LocationBarControllerImpl>(
          self.viewController.omnibox, browser->browser_state(),
          nil /* PreloadProvider */, nil /* OmniboxPopupPositioner */,
          self.mediator, nil /* dispatcher */);
  OmniboxPopupViewIOS* popupView = locationBar->GetPopupView();
  self.popupCoordinator =
      [[PopupCoordinator alloc] initWithPopupView:popupView];
  [self addChildCoordinator:self.popupCoordinator];
  [self.mediator setLocationBar:std::move(locationBar)];
  [super start];
}

- (void)stop {
  [super stop];
  [self.browser->dispatcher()
      stopDispatchingForProtocol:@protocol(PopupDisplaying)];
  self.viewController = nil;
  self.mediator = nil;
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  if ([childCoordinator isKindOfClass:[PopupCoordinator class]]) {
    id<ABC> tab = (id<ABC>)self.browser->dispatcher();
    PopupCoordinator* popupCoordinator = (PopupCoordinator*)childCoordinator;
    [tab fooBarBaz:popupCoordinator.viewController];
  }
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  if ([childCoordinator isKindOfClass:[PopupCoordinator class]]) {
    id<ABC> tab = (id<ABC>)self.browser->dispatcher();
    [tab fooBarBaz:nil];
  }
}

#pragma mark - PopupDisplaying

- (void)showPopup {
  [self.popupCoordinator start];
}

- (void)hidePopup {
  [self.popupCoordinator stop];
}

@end
