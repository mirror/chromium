// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/public/chrome_web_view.h"

#include "base/strings/sys_string_conversions.h"
#include "google_apis/google_api_keys.h"
#include "ios/web_view/internal/web_view_global_state_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

static NSString* gUserAgentProduct = nil;

@implementation ChromeWebView

+ (NSString*)userAgentProduct {
  return gUserAgentProduct;
}

+ (void)setUserAgentProduct:(NSString*)product {
  gUserAgentProduct = [product copy];
}

+ (void)setGoogleAPIKey:(NSString*)googleAPIKey
               clientID:(NSString*)clientID
           clientSecret:(NSString*)clientSecret {
  google_apis::SetAPIKey(base::SysNSStringToUTF8(googleAPIKey));

  std::string clientIDString = base::SysNSStringToUTF8(clientID);
  std::string clientSecretString = base::SysNSStringToUTF8(clientSecret);
  for (size_t i = 0; i < google_apis::CLIENT_NUM_ITEMS; ++i) {
    google_apis::OAuth2Client client =
        static_cast<google_apis::OAuth2Client>(i);
    google_apis::SetOAuth2ClientID(client, clientIDString);
    google_apis::SetOAuth2ClientSecret(client, clientSecretString);
  }
}

+ (void)initializeGlobalState {
  ios_web_view::InitializeGlobalState();
}

@end
