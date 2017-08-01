// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler.h"

#import "base/strings/sys_string_conversions.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Convenience function to add key with unescaped value to input URL.
GURL AppendQueryParameter(const GURL& input,
                          const std::string& key,
                          const std::string& value) {
  std::string query(input.query());
  if (!query.empty())
    query += "&";
  query += key + "=" + value;
  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return input.ReplaceComponents(replacements);
}

}  // namespace

@implementation MailtoHandler {
  NSSet<NSString*>* _supportedHeaders;
}

@synthesize appName = _appName;
@synthesize appStoreID = _appStoreID;

- (instancetype)initWithName:(NSString*)appName
                  appStoreID:(NSString*)appStoreID {
  self = [super init];
  if (self) {
    _appName = [appName copy];
    _appStoreID = [appStoreID copy];
    _supportedHeaders = [NSSet<NSString*>
        setWithObjects:@"to", @"cc", @"bcc", @"subject", @"body", nil];
  }
  return self;
}

- (BOOL)isAvailable {
  NSURL* baseURL = [NSURL URLWithString:[self beginningScheme]];
  NSURL* testURL = [NSURL
      URLWithString:[NSString stringWithFormat:@"%@://", [baseURL scheme]]];
  return [[UIApplication sharedApplication] canOpenURL:testURL];
}

- (NSString*)beginningScheme {
  // Subclasses should override this method.
  return @"mailtohandler:/co?";
}

- (NSSet<NSString*>*)supportedHeaders {
  return _supportedHeaders;
}

- (NSString*)rewriteMailtoURL:(const GURL&)gURL {
  if (!gURL.SchemeIs(url::kMailToScheme))
    return nil;
  GURL result(base::SysNSStringToUTF8([self beginningScheme]));
  if (gURL.path().length())
    result = AppendQueryParameter(result, "to", gURL.path());
  net::QueryIterator it(gURL);
  while (!it.IsAtEnd()) {
    // Normalizes the keys to all lowercase, but keeps the value unchanged.
    NSString* keyString =
        [base::SysUTF8ToNSString(it.GetKey()) lowercaseString];
    if ([[self supportedHeaders] containsObject:keyString]) {
      result = AppendQueryParameter(result, base::SysNSStringToUTF8(keyString),
                                    it.GetValue());
    }
    it.Advance();
  }
  return base::SysUTF8ToNSString(result.spec());
}

@end
