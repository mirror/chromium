// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/activity_service_coordinator.h"

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/passwords/password_controller.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/activity_services/activity_service_controller.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_positioner.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_presentation.h"
#import "ios/chrome/browser/ui/activity_services/requirements/activity_service_snackbar.h"
#import "ios/chrome/browser/ui/activity_services/share_protocol.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data_builder.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/shared/chrome/browser/ui/commands/command_dispatcher.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ActivityServiceCoordinator ()<ShareToDelegate>
@end

@implementation ActivityServiceCoordinator

@synthesize browserState = _browserState;
@synthesize dispatcher = _dispatcher;
@synthesize tabModel = _tabModel;

@synthesize positionProvider = _positionProvider;
@synthesize presentationProvider = _presentationProvider;
@synthesize snackbarProvider = _snackbarProvider;

- (void)disconnect {
  self.browserState = nil;
  self.dispatcher = nil;
}

- (void)setDispatcher:(CommandDispatcher*)dispatcher {
  if (dispatcher == self.dispatcher) {
    return;
  }

  if (self.dispatcher) {
    [self.dispatcher stopDispatchingToTarget:self];
  }

  [dispatcher startDispatchingToTarget:self forSelector:@selector(sharePage)];
  _dispatcher = dispatcher;
}

- (void)sharePage {
  ShareToData* data =
      activity_services::ShareToDataForTab([self.tabModel currentTab]);
  if (data)
    [self sharePageWithData:data];
}

- (void)sharePageWithData:(ShareToData*)data {
  id<ShareProtocol> controller = [ActivityServiceController sharedInstance];
  if ([controller isActive])
    return;
  CGRect fromRect = [self.positionProvider shareButtonAnchorRect];
  UIView* inView = [self.positionProvider shareButtonView];
  [controller shareWithData:data
                 controller:self.baseViewController
               browserState:_browserState
            shareToDelegate:self
                   fromRect:fromRect
                     inView:inView];
}

- (void)cancelShare {
  id<ShareProtocol> controller = [ActivityServiceController sharedInstance];
  [controller cancelShareAnimated:NO];
}

- (void)passwordAppExDidFinish:(ShareTo::ShareResult)shareStatus
                      username:(NSString*)username
                      password:(NSString*)password
             completionMessage:(NSString*)message {
  switch (shareStatus) {
    case ShareTo::SHARE_SUCCESS: {
      PasswordController* passwordController =
          [[self.tabModel currentTab] passwordController];
      __block BOOL shown = NO;
      [passwordController findAndFillPasswordForms:username
                                          password:password
                                 completionHandler:^(BOOL completed) {
                                   if (shown || !completed || ![message length])
                                     return;
                                   TriggerHapticFeedbackForNotification(
                                       UINotificationFeedbackTypeSuccess);
                                   [self.snackbarProvider showSnackbar:message];
                                   shown = YES;
                                 }];
      break;
    }
    default:
      break;
  }
}

- (void)shareDidComplete:(ShareTo::ShareResult)shareStatus
       completionMessage:(NSString*)message {
  // The shareTo dialog dismisses itself instead of through
  // |-dismissViewControllerAnimated:completion:| so we must reset the
  // presenting state here.
  [self.presentationProvider activityServiceDidEndPresenting];

  switch (shareStatus) {
    case ShareTo::SHARE_SUCCESS:
      if ([message length]) {
        TriggerHapticFeedbackForNotification(UINotificationFeedbackTypeSuccess);
        [self.snackbarProvider showSnackbar:message];
      }
      break;
    case ShareTo::SHARE_ERROR:
      [self showErrorAlert:IDS_IOS_SHARE_TO_ERROR_ALERT_TITLE
                   message:IDS_IOS_SHARE_TO_ERROR_ALERT];
      break;
    case ShareTo::SHARE_NETWORK_FAILURE:
      [self showErrorAlert:IDS_IOS_SHARE_TO_NETWORK_ERROR_ALERT_TITLE
                   message:IDS_IOS_SHARE_TO_NETWORK_ERROR_ALERT];
      break;
    case ShareTo::SHARE_SIGN_IN_FAILURE:
      [self showErrorAlert:IDS_IOS_SHARE_TO_SIGN_IN_ERROR_ALERT_TITLE
                   message:IDS_IOS_SHARE_TO_SIGN_IN_ERROR_ALERT];
      break;
    case ShareTo::SHARE_CANCEL:
    case ShareTo::SHARE_UNKNOWN_RESULT:
      break;
  }
}

- (void)showErrorAlert:(int)titleMessageId message:(int)messageId {
  NSString* title = l10n_util::GetNSString(titleMessageId);
  NSString* message = l10n_util::GetNSString(messageId);
  [self.presentationProvider showErrorAlertWithStringTitle:title
                                                   message:message];
}

@end
