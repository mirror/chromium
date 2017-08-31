// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/activity_services/activity_service_coordinator.h"

#import "ios/clean/chrome/browser/ui/commands/activity_services_commands.h"
#import "ios/shared/chrome/browser/ui/browser_list/browser.h"
#import "ios/shared/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/activity_services/activity_service_controller.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_password.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_positioner.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_presentation.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_snackbar.h"
#import "ios/chrome/browser/ui/activity_services/share_protocol.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data_builder.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/shared/chrome/browser/ui/commands/command_dispatcher.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ActivityServiceCleanCoordinator ()<ActivityServicePassword,
                                              ActivityServicePositioner,
                                              ActivityServicePresentation,
                                              ActivityServiceSnackbar,
                                              ActivityServicesCommands>

@property(nonatomic, strong) UIViewController* viewController;

@end

@implementation ActivityServiceCleanCoordinator

@synthesize viewController = _viewController;

- (void)wasAddedToParentCoordinator:(BrowserCoordinator*)parent {
  CommandDispatcher* dispatcher = self.browser->dispatcher();
  [dispatcher startDispatchingToTarget:self forSelector:@selector(sharePage)];
}

- (void)willBeRemovedFromParentCoordinator {
  CommandDispatcher* dispatcher = self.browser->dispatcher();
  [dispatcher stopDispatchingToTarget:self];
}

- (void)sharePage {
  NSLog(@"sharePage");

#if 0
  ShareToData* data =
      activity_services::ShareToDataForTab(nil);
#else
  ShareToData* data =
      [[ShareToData alloc] initWithURL:GURL("https://www.google.com")
                                 title:@"Share Page Title"
                       isOriginalTitle:YES
                       isPagePrintable:YES
                    thumbnailGenerator:^UIImage*(const CGSize&) {
                      return nil;
                    }];
#endif
  if (!data)
    return;

  id<ShareProtocol> controller = [ActivityServiceController sharedInstance];
  if ([controller isActive])
    return;

  [controller shareWithData:data
               browserState:self.browser->browser_state()
           passwordProvider:self
           positionProvider:self
       presentationProvider:self
           snackbarProvider:self];
}

- (void)cancelShare {
  id<ShareProtocol> controller = [ActivityServiceController sharedInstance];
  [controller cancelShareAnimated:NO];
}

#pragma mark - Providers

- (PasswordController*)currentPasswordController {
  return nil;
}

- (void)presentActivityServiceViewController:(UIViewController*)controller {
  self.viewController = controller;
  [self start];
}

- (void)activityServiceDidEndPresenting {
  self.viewController = nil;
  [self stop];
}

- (void)showErrorAlertWithStringTitle:(NSString*)title
                              message:(NSString*)message {
}

- (CGRect)shareButtonAnchorRect {
  return CGRectZero;
}

- (UIView*)shareButtonView {
  return nil;
}

- (void)showSnackbar:(NSString*)message {
}

@end
