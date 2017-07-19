// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_

#import <UIKit/UIKit.h>

namespace ios {
class ChromeBrowserState;
}  // namespace ios

namespace web {
class WebState;
}  // namespace web

// Implements the app-side of the Credential Manager JavaScript API for a single
// WebState.
// TODO(crbug.com/435047) tgarbus: This should provide proper methods (Get,
// Store, PreventSilentAccess) to be invoked by message handlers from
// CRWWebController. After performing requested action it should invoke a method
// from JSCredentialManager responsible for resolving or rejecting a promise.
// TODO(crbug.com/435045) tgarbus: This should also be responsible for invoking
// credential management UI
@interface CredentialManager : NSObject

// The current web state being observed for Credential Manager API invocations.
@property(nonatomic, assign) web::WebState* webState;

// The ios::ChromeBrowserState instance passed to the initializer.
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;

// Designated initializer.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
                            webState:(web::WebState*)webState
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_
