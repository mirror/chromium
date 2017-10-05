// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_COORDINATOR_H_

#import <UIKit/UIKit.h>

#include "components/signin/core/browser/signin_metrics.h"
#import "ios/chrome/browser/chrome_coordinator.h"
#include "ios/chrome/browser/signin/constants.h"

@protocol ApplicationCommands;
namespace ios {
class ChromeBrowserState;
}
@class ChromeIdentity;

@interface SigninInteractionCoordinator : ChromeCoordinator

// Designated initializer.
// * |browserState| is the current browser state
// * |presentingViewController| is the top presented view controller.
// * |isPresentedOnSettings| indicates whether the settings view controller is
//   part of the presented view controllers stack
// * |accessPoint| represents the access point that initiated the sign-in.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
                              browserState:
                                  (ios::ChromeBrowserState*)browserState
                                dispatcher:(id<ApplicationCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithBaseViewController:(UIViewController*)viewController
    NS_UNAVAILABLE;

// Starts user sign-in.
// * |identity|, if not nil, the user will be signed in without requiring user
//   input, using this Chrome identity.
// * |completion| will be called when the operation is done, and
//   |succeeded| will notify the caller on whether the user is now signed in.
- (void)signInWithIdentity:(ChromeIdentity*)identity
               accessPoint:(signin_metrics::AccessPoint)accessPoint
               promoAction:(signin_metrics::PromoAction)promoAction
     isPresentedOnSettings:(BOOL)isPresentedOnSettings
                completion:(signin_ui::CompletionCallback)completion;

// Re-authenticate the user. This method will always show a sign-in web flow.
// The completion block will be called when the operation is done, and
// |succeeded| will notify the caller on whether the user has been
// correctly re-authenticated.
- (void)reAuthenticateWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                          promoAction:(signin_metrics::PromoAction)promoAction
                isPresentedOnSettings:(BOOL)isPresentedOnSettings
                           completion:(signin_ui::CompletionCallback)completion;

// Starts the flow to add an identity via ChromeIdentityInteractionManager.
- (void)addAccountWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                      promoAction:(signin_metrics::PromoAction)promoAction
            isPresentedOnSettings:(BOOL)isPresentedOnSettings
                       completion:(signin_ui::CompletionCallback)completion;

// Cancels any current process. Calls the completion callback when done.
- (void)cancel;

// Cancels any current process and dismisses any UI. Calls the completion
// callback when done.
- (void)cancelAndDismiss;

@property(nonatomic, assign, readonly) BOOL signinInProgress;

@end

#endif  // IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_COORDINATOR_H_
