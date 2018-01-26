// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/crw_context_menu_element_fetch_details.h"

#include "base/time/time.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CRWContextMenuElementFetchDetails

@synthesize fetchStartTime = _fetchStartTime;
@synthesize foundElementHandler = _foundElementHandler;
@synthesize identifier = _identifier;

- (instancetype)initWithFoundElementHandler:
    (void (^)(NSDictionary*))foundElementHandler {
  self = [super init];
  if (self) {
    _fetchStartTime = base::TimeTicks::Now();
    _foundElementHandler = foundElementHandler;
    _identifier = [[NSProcessInfo processInfo] globallyUniqueString];
  }
  return self;
}

@end
