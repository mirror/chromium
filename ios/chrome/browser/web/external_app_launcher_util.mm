// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_app_launcher_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* GetFormattedAbsoluteUrlWithSchemeRemoved(NSURL* url) {
  NSCharacterSet* char_set =
      [NSCharacterSet characterSetWithCharactersInString:@"/"];
  NSString* scheme = [NSString stringWithFormat:@"%@:", url.scheme];
  NSString* url_string = url.absoluteString;
  if (url_string.length <= scheme.length)
    return url_string;
  NSString* prompt = [[[url.absoluteString substringFromIndex:scheme.length]
      stringByTrimmingCharactersInSet:char_set]
      stringByRemovingPercentEncoding];
  // Returns original URL string if there's nothing interesting to display
  // other than the scheme itself.
  if (!prompt.length)
    return url_string;
  return prompt;
}
