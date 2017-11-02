// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_GETTER_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_GETTER_DELEGATE_H_

#import <Foundation/Foundation.h>

@class CanonicalURLGetter;
class GURL;

// Delegate for |CanonicalURLGetter| that is notified when a canonical URL is
// retrieved.
@protocol CanonicalURLGetterDelegate<NSObject>

// Called when a |canonicalURL| is retrieved by |getter|.
// |canonicalURL| can be an empty URL under various circumstances. See
// |CanonicalURLGetter| documentation for more details.
- (void)canonicalURLGetter:(CanonicalURLGetter*)getter
    didRetrieveCanonicalURL:(const GURL&)canonicalURL;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_CANONICAL_URL_GETTER_DELEGATE_H_
