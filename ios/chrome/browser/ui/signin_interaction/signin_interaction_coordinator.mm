// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/signin_interaction/signin_interaction_coordinator.h"

#import "base/ios/block_types.h"
#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"
#import "ios/chrome/browser/ui/authentication/authentication_ui_util.h"
#import "ios/chrome/browser/ui/signin_interaction/signin_interaction_controller.h"
#import "ios/chrome/browser/ui/signin_interaction/signin_interaction_presenting.h"
#import "ios/chrome/browser/ui/util/top_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SigninInteractionCoordinator ()<SigninInteractionPresenting>

@property(nonatomic, strong) AlertCoordinator* alertCoordinator;
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
@property(nonatomic, strong) SigninInteractionController* controller;
@property(nonatomic, weak) id<ApplicationCommands> dispatcher;
@property(nonatomic, assign) BOOL isPresentingOnSettings;

@end

@implementation SigninInteractionCoordinator
@synthesize alertCoordinator = _alertCoordinator;
@synthesize browserState = _browserState;
@synthesize controller = _controller;
@synthesize dispatcher = _dispatcher;
@synthesize signinInProgress = _signinInProgress;
@synthesize isPresenting = _isPresenting;
@synthesize isPresentingOnSettings = _isPresentingOnSettings;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                              browserState:
                                  (ios::ChromeBrowserState*)browserState
                                dispatcher:(id<ApplicationCommands>)dispatcher {
  self = [super initWithBaseViewController:viewController];
  if (self) {
    _browserState = browserState;
    _dispatcher = dispatcher;
  }
  return self;
}

- (void)signInWithIdentity:(ChromeIdentity*)identity
               accessPoint:(signin_metrics::AccessPoint)accessPoint
               promoAction:(signin_metrics::PromoAction)promoAction
     isPresentedOnSettings:(BOOL)isPresentedOnSettings
                completion:(signin_ui::CompletionCallback)completion {
  if (self.controller) {
    return;
  }

  [self setupForSigninOperationWithAccessPoint:accessPoint
                                   promoAction:promoAction
                         isPresentedOnSettings:isPresentedOnSettings];

  [self.controller
      signInWithIdentity:identity
              completion:[self callbackForClearingControllerWithCompletion:
                                   completion]];
}

- (void)reAuthenticateWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                          promoAction:(signin_metrics::PromoAction)promoAction
                isPresentedOnSettings:(BOOL)isPresentedOnSettings
                           completion:
                               (signin_ui::CompletionCallback)completion {
  if (self.controller) {
    return;
  }

  [self setupForSigninOperationWithAccessPoint:accessPoint
                                   promoAction:promoAction
                         isPresentedOnSettings:isPresentedOnSettings];

  [self.controller
      reAuthenticateWithCompletion:
          [self callbackForClearingControllerWithCompletion:completion]];
}

- (void)addAccountWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                      promoAction:(signin_metrics::PromoAction)promoAction
            isPresentedOnSettings:(BOOL)isPresentedOnSettings
                       completion:(signin_ui::CompletionCallback)completion {
  if (self.controller) {
    return;
  }

  [self setupForSigninOperationWithAccessPoint:accessPoint
                                   promoAction:promoAction
                         isPresentedOnSettings:isPresentedOnSettings];

  [self.controller
      addAccountWithCompletion:
          [self callbackForClearingControllerWithCompletion:completion]];
}

- (void)cancel {
  [self.controller cancel];
}

- (void)cancelAndDismiss {
  [self.controller cancelAndDismiss];
}

#pragma mark - SigninInteractionPresenting

- (void)presentViewController:(UIViewController*)viewController
                     animated:(BOOL)animated
                   completion:(ProceduralBlock)completion {
  [self.baseViewController presentViewController:viewController
                                        animated:animated
                                      completion:completion];
}

- (void)presentTopViewController:(UIViewController*)viewController
                        animated:(BOOL)animated
                      completion:(ProceduralBlock)completion {
  // TODO(crbug.com/754642): Stop using TopPresentedViewControllerFrom().
  UIViewController* topController =
      top_view_controller::TopPresentedViewControllerFrom(
          self.baseViewController);
  [topController presentViewController:viewController
                              animated:animated
                            completion:completion];
}

- (void)dismissViewControllerAnimated:(BOOL)animated
                           completion:(ProceduralBlock)completion {
  [self.baseViewController dismissViewControllerAnimated:animated
                                              completion:completion];
}

- (void)presentError:(NSError*)error
       dismissAction:(ProceduralBlock)dismissAction {
  DCHECK(!self.alertCoordinator);
  self.alertCoordinator =
      ErrorCoordinator(error, dismissAction,
                       top_view_controller::TopPresentedViewControllerFrom(
                           self.baseViewController));
  [self.alertCoordinator start];
}

- (void)dismissError {
  [self.alertCoordinator executeCancelHandler];
  [self.alertCoordinator stop];
  self.alertCoordinator = nil;
}

#pragma mark - Private Methods

- (void)
setupForSigninOperationWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                           promoAction:(signin_metrics::PromoAction)promoAction
                 isPresentedOnSettings:(BOOL)isPresentedOnSettings {
  self.isPresentingOnSettings = isPresentedOnSettings;

  self.controller = [[SigninInteractionController alloc]
      initWithBrowserState:self.browserState
      presentationProvider:self
               accessPoint:accessPoint
               promoAction:promoAction
                dispatcher:self.dispatcher];
}

- (signin_ui::CompletionCallback)callbackForClearingControllerWithCompletion:
    (signin_ui::CompletionCallback)completion {
  __weak SigninInteractionCoordinator* weakSelf = self;
  signin_ui::CompletionCallback completionCallback = ^(BOOL success) {
    weakSelf.controller = nil;
    if (completion) {
      completion(success);
    }
  };
  return completionCallback;
}

@end
