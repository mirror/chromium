// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_OPEN_URL_UTIL_H_
#define IOS_CHROME_BROWSER_OPEN_URL_UTIL_H_

#import <Foundation/Foundation.h>

// Launch another app to handle |url|. |completion_handler| will be called on
// the main thread with the status of whether another app was launched.
void OpenUrlWithCompletionHandler(NSURL* url, void (^completion_handler)(BOOL));

#endif  // IOS_CHROME_BROWSER_OPEN_URL_UTIL_H_
