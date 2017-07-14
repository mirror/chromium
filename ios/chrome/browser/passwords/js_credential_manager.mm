// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/js_credential_manager.h"

#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface JSCredentialManager ()

- (void)executeScript:(NSString*)script
    completionHandler:(ProceduralBlockWithBool)completionHandler;

@end

@implementation JSCredentialManager

- (void)executeNoop {
  [self executeScript:@"Function.prototype()" completionHandler:nil];
}

- (void)executeScript:(NSString*)script
    completionHandler:(ProceduralBlockWithBool)completionHandler {
  [self executeJavaScript:script
        completionHandler:^(id result, NSError* error) {
          if (completionHandler)
            completionHandler(!error);
        }];
}

- (void)resolveResponsePromiseWithCompletionHandler:
    (ProceduralBlockWithBool)completionHandler {
}

#pragma mark - Protected methods

- (NSString*)scriptPath {
  return @"credential_manager";
}

@end
