// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CHROME_WEB_VIEW_H_
#define IOS_WEB_VIEW_PUBLIC_CHROME_WEB_VIEW_H_

#import <Foundation/Foundation.h>

#import "cwv_export.h"

// Class which manages the framework's global state.
CWV_EXPORT
@interface ChromeWebView : NSObject

// The User Agent product string used to build the full User Agent.
+ (NSString*)userAgentProduct;

// Customizes the User Agent string by inserting |product|. It should be of the
// format "product/1.0". For example:
// "Mozilla/5.0 (iPhone; CPU iPhone OS 10_3 like Mac OS X) AppleWebKit/603.1.30
// (KHTML, like Gecko) <product> Mobile/16D32 Safari/602.1" where <product>
// will be replaced with |product| or empty string if not set.
//
// NOTE: It is recommended to set |product| before initializing any web views.
// Setting |product| is only guaranteed to affect web views which have not yet
// been initialized. However, exisiting web views could also be affected
// depending upon their internal state.
+ (void)setUserAgentProduct:(NSString*)product;

// Use this method to set the necessary credentials used to communicate with
// the Google API for features such as translate. See this link for more info:
// https://support.google.com/googleapi/answer/6158857
// This method must be called before calling |initializeGlobalState|.
+ (void)setGoogleAPIKey:(NSString*)googleAPIKey
               clientID:(NSString*)clientID
           clientSecret:(NSString*)clientSecret;

// Sets up the required global state. Must be called before interacting with any
// ChromeWebView framework classes. This includes |Cronet| if using the
// CronetChromeWebView framework.
+ (void)initializeGlobalState;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CHROME_WEB_VIEW_H_
