// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_IDENTITY_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_IDENTITY_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

CWV_EXPORT
@interface CWVIdentity : NSObject

// Identity/account email address. This can be shown to the user, but is not a
// unique identifier (@see gaiaID).
@property(strong, nonatomic) NSString* userEmail;

// The unique GAIA user identifier for this identity/account.
// You may use this as a unique identifier to remember a particular identity.
@property(strong, nonatomic) NSString* gaiaID;

// Returns the full name of the identity.
// Could be nil if no full name has been fetched for this account yet.
@property(strong, nonatomic) NSString* userFullName;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_IDENTITY_H_
