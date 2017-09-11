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

- (void)authenticationController:(CWVAuthenticationController*)controller
         getAccessTokenForGaiaID:(NSString*)gaiaID
                        clientID:(NSString*)clientID
                    clientSecret:(NSString*)clientSecret
                          scopes:(NSArray<NSString*>*)scopes
                        callback:
                            (void (^)(NSString*, NSDate*, NSError*))callback;

- (NSArray<CWVIdentity*>*)authenticationControllerGetAllIdentities:
    (CWVAuthenticationController*)controller;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_DELEGATE_H
