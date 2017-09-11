// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_H
#define IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_H

#import <Foundation/Foundation.h>

#import "cwv_export.h"

@protocol CWVAuthenticationControllerDelegate;
@class CWVIdentity;

// Controller used to manage authentication.
CWV_EXPORT
@interface CWVAuthenticationController : NSObject

// Delegate used to provide account information.
@property(nonatomic, weak) id<CWVAuthenticationControllerDelegate> delegate;

// The signed in user, if any.
@property(nonatomic, readonly) CWVIdentity* currentIdentity;

- (instancetype)init NS_UNAVAILABLE;

// Logs in with |identity| into ChromeWebView.
- (void)signInWithIdentity:(CWVIdentity*)identity;

// Logs out |currentIdentity|.
- (void)signOut;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_AUTHENTICATION_CONTROLLER_H
