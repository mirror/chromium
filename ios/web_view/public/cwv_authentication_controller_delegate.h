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
// |clientSecret| The clientSecret of ChromeWebView. Use like |clientID|.
// |scopes| The oauth scopes requested.
// |callback| Block used to return access token, expiration date, and error.
- (void)authenticationController:(CWVAuthenticationController*)controller
         getAccessTokenForGaiaID:(NSString*)gaiaID
                        clientID:(NSString*)clientID
                    clientSecret:(NSString*)clientSecret
                          scopes:(NSArray<NSString*>*)scopes
                        callback:
                            (void (^)(NSString*, NSDate*, NSError*))callback;

// Called when ChromeWebView needs a list of all SSO identities.
// Every identity you return here will be reflected in google web properties.
// You must return the signed in user at the very minimum.
- (NSArray<CWVIdentity*>*)authenticationControllerGetAllIdentities:
    (CWVAuthenticationController*)controller;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H
