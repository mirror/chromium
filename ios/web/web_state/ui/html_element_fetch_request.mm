// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/html_element_fetch_request.h"

#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface HTMLElementFetchRequest ()
// Completion handler to call with found DOM element.
@property(nonatomic, copy) void (^foundElementHandler)(NSDictionary*);
@end

@implementation HTMLElementFetchRequest

@synthesize creationTime = _creationTime;
@synthesize foundElementHandler = _foundElementHandler;
@synthesize identifier = _identifier;

- (instancetype)initWithFoundElementHandler:
    (void (^)(NSDictionary*))foundElementHandler {
  self = [super init];
  if (self) {
    _creationTime = base::TimeTicks::Now();
    _foundElementHandler = foundElementHandler;
    _identifier =
        base::SysUTF8ToNSString(base::UnguessableToken::Create().ToString());
  }
  return self;
}

- (void)runHandlerWithResponse:(NSDictionary*)response {
  _foundElementHandler(response);
}

@end
