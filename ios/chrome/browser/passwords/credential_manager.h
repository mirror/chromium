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

// Manager for handling invocations of the Credential Manager API.
//
// Implements the app-side of the  Credential Manager JavaScript API. Injects
// and listens to the injected JavaScript.
// TODO(crbug.com/435047) tgarbus: This should bind JSCredentialManager and
// CredentialManagerImpl
// TODO(crbug.com/435045) tgarbus: This should also be responsible for invoking
// credential management UI
@interface CredentialManager : NSObject

// The current web state being observed for Credential Manager API invocations.
@property(nonatomic, assign) web::WebState* webState;

// The ios::ChromeBrowserState instance passed to the initializer.
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;

// Designated initializer.
- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Sets the WebState to be observed for invocations of the Credential Manager
// API.
- (void)setWebState:(web::WebState*)webState;

@end

#endif  // IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_
