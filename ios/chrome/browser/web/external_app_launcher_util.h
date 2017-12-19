// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_UTIL_H_
#define IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_UTIL_H_

#import <Foundation/Foundation.h>

// Returns a formatted string that removes the url scheme (e.g., "tel://") and
// percent encoding from the absolute string of |url|.
NSString* GetFormattedAbsoluteUrlWithSchemeRemoved(NSURL* url);

#endif  // IOS_CHROME_BROWSER_WEB_EXTERNAL_APP_LAUNCHER_UTIL_H_
