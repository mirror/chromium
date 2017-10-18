// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PROVIDERS_CHROMIUM_EXTERNAL_SEARCH_PROVIDER_H_
#define IOS_CHROME_BROWSER_PROVIDERS_CHROMIUM_EXTERNAL_SEARCH_PROVIDER_H_

#import <Foundation/Foundation.h>

#import "ios/public/provider/chrome/browser/external_search/external_search_provider.h"

class ChromiumExternalSearchProvider : public ExternalSearchProvider {
 public:
  NSURL* GetLaunchURL() override;
  NSString* GetButtonImageName() override;
  int GetButtonIdsAccessibilityLabel() override;
  NSString* GetButtonAccessibilityIdentifier() override;
};

#endif  // IOS_CHROME_BROWSER_PROVIDERS_CHROMIUM_EXTERNAL_SEARCH_PROVIDER_H_
