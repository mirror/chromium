// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/test/fakes/crw_test_back_forward_list.h"

#import <WebKit/WebKit.h>
#include "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWTestBackForwardList (PrivateMethods)
- (WKBackForwardListItem*)mockItemWithURLString:(NSString*)url;
- (NSArray*)mockSublistWithURLArray:(NSArray<NSString*>*)urls;
@end

@implementation CRWTestBackForwardList

@synthesize backList;
@synthesize forwardList;
@synthesize currentItem;

- (WKBackForwardListItem*)itemAtIndex:(NSInteger)index {
  if (index == 0) {
    return self.currentItem;
  } else if (index > 0 && self.forwardList.count) {
    return self.forwardList[index - 1];
  } else if (self.backList.count) {
    return self.backList[self.backList.count + index];
  }
  return nil;
}

- (void)setCurrentURL:(NSString*)currentItemURL {
  [self setCurrentURL:currentItemURL backListURLs:nil forwardListURLs:nil];
}

- (void)setCurrentURL:(NSString*)currentItemURL
         backListURLs:(nullable NSArray<NSString*>*)backListURLs
      forwardListURLs:(nullable NSArray<NSString*>*)forwardListURLs {
  self.currentItem = [self mockItemWithURLString:currentItemURL];
  self.backList = [self mockSublistWithURLArray:backListURLs];
  self.forwardList = [self mockSublistWithURLArray:forwardListURLs];
}

- (WKBackForwardListItem*)mockItemWithURLString:(NSString*)url {
  id mock = OCMClassMock([WKBackForwardListItem class]);
  OCMStub([mock URL]).andReturn([NSURL URLWithString:url]);
  return mock;
}

- (NSArray*)mockSublistWithURLArray:(NSArray<NSString*>*)urls {
  NSMutableArray* array = [NSMutableArray arrayWithCapacity:urls.count];
  for (NSString* url : urls) {
    [array addObject:[self mockItemWithURLString:url]];
  }
  return [NSArray arrayWithArray:array];
}

@end
