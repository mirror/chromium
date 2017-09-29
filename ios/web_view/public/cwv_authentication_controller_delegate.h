// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#import "cwv_export.h"

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H

NS_ASSUME_NONNULL_BEGIN

@class CWVAuthenticationController;

CWV_EXPORT
@protocol CWVAuthenticationControllerDelegate<NSObject>

// Called when ChromeWebView needs an access token.
// |gaiaID| The GaiaID of the user whose access token is requested.
// |clientID| The clientID of ChromeWebView. Used to verify it is the same as
// the one passed to CWVWebView and SSO.
// |clientSecret| The clientSecret of ChromeWebView. Used like |clientID|.
// |scopes| The OAuth scopes requested.
// |completionHandler| Used to return access tokens, expiration date, and error.
- (void)authenticationController:(CWVAuthenticationController*)controller
         getAccessTokenForGaiaID:(NSString*)gaiaID
                        clientID:(NSString*)clientID
                    clientSecret:(NSString*)clientSecret
                          scopes:(NSArray<NSString*>*)scopes
               completionHandler:
                   (void (^)(NSString*, NSDate*, NSError*))completionHandler;

// Called when ChromeWebView needs a list of all SSO identities.
// Every identity returned here may be reflected in google web properties.
// This will not be called unless you are signed in.
- (NSArray<CWVIdentity*>*)authenticationControllerGetAllIdentities:
    (CWVAuthenticationController*)controller;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H
