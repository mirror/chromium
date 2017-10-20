// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_OPEN_URL_UTIL_H_
#define IOS_CHROME_BROWSER_OPEN_URL_UTIL_H_

#import <Foundation/Foundation.h>

#import "ios/web/public/web_state/ui/crw_web_delegate.h"

// Wrapper method for UIApplication -openURL: that uses
// the non-deprecated method when it is available (iOS10+).
void OpenUrlWithCompletionHandler(NSURL* url,
                                  OpenURLCompletionBlock completion_handler);

#endif  // IOS_CHROME_BROWSER_OPEN_URL_UTIL_H_
