// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_COORDINATOR_H_

#import <UIKit/UIKit.h>

#include "components/signin/core/browser/signin_metrics.h"
#include "ios/chrome/browser/signin/constants.h"

@protocol ApplicationCommands;
namespace ios {
class ChromeBrowserState;
}
@class ChromeIdentity;

// The coordinator for the SignInInteractionController. This coordinator handles
// presentation and dismissal of its UI. But, if it is destroyed while
// |signinInProgress|, it will not dismiss its presented UI.
@interface SigninInteractionCoordinator : NSObject

// All method comments need to be updated.

// Designated initializer.
// * |browserState| is the current browser state
// * |presentingViewController| is the top presented view controller.
// * |isPresentedOnSettings| indicates whether the settings view controller is
//   part of the presented view controllers stack
// * |accessPoint| represents the access point that initiated the sign-in.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                          dispatcher:(id<ApplicationCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Starts user sign-in.
// * |identity|, if not nil, the user will be signed in without requiring user
//   input, using this Chrome identity.
// * |completion| will be called when the operation is done, and
//   |succeeded| will notify the caller on whether the user is now signed in.
- (void)signInWithIdentity:(ChromeIdentity*)identity
                 accessPoint:(signin_metrics::AccessPoint)accessPoint
                 promoAction:(signin_metrics::PromoAction)promoAction
    presentingViewController:(UIViewController*)viewController
                  completion:(signin_ui::CompletionCallback)completion;

// Re-authenticate the user. This method will always show a sign-in web flow.
// The completion block will be called when the operation is done, and
// |succeeded| will notify the caller on whether the user has been
// correctly re-authenticated.
- (void)reAuthenticateWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                          promoAction:(signin_metrics::PromoAction)promoAction
             presentingViewController:(UIViewController*)viewController
                           completion:(signin_ui::CompletionCallback)completion;

// Starts the flow to add an identity via ChromeIdentityInteractionManager.
- (void)addAccountWithAccessPoint:(signin_metrics::AccessPoint)accessPoint
                      promoAction:(signin_metrics::PromoAction)promoAction
         presentingViewController:(UIViewController*)viewController
                       completion:(signin_ui::CompletionCallback)completion;

// Cancels any current process. Calls the completion callback when done.
- (void)cancel;

// Cancels any current process and dismisses any UI. Calls the completion
// callback when done.
- (void)cancelAndDismiss;

@property(nonatomic, assign, readonly) BOOL signinInProgress;

@end

#endif  // IOS_CHROME_BROWSER_UI_SIGNIN_INTERACTION_SIGNIN_INTERACTION_COORDINATOR_H_
