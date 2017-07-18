// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_back_forward_list_mock.h"

#import <WebKit/WebKit.h>
#include "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWBackForwardListMock (PrivateMethods)
- (WKBackForwardListItem*)createMockItem:(NSString*)url;
- (NSArray*)createMockSublist:(NSArray<NSString*>*)urls;
@end

@implementation CRWBackForwardListMock

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

- (void)setCurrentUrl:(NSString*)currentItemUrl {
  [self setCurrentUrl:currentItemUrl backListUrls:nil forwardListUrls:nil];
}

- (void)setCurrentUrl:(NSString*)currentItemUrl
         backListUrls:(NSArray<NSString*>*)backListUrls
      forwardListUrls:(NSArray<NSString*>*)forwardListUrls {
  self.currentItem = [self createMockItem:currentItemUrl];
  self.backList = [self createMockSublist:backListUrls];
  self.forwardList = [self createMockSublist:forwardListUrls];
}

- (WKBackForwardListItem*)createMockItem:(NSString*)url {
  id mock = OCMClassMock([WKBackForwardListItem class]);
  OCMStub([mock URL]).andReturn([NSURL URLWithString:url]);
  return mock;
}

- (NSArray*)createMockSublist:(NSArray<NSString*>*)urls {
  NSMutableArray* array = [NSMutableArray arrayWithCapacity:urls.count];
  for (NSString* url : urls) {
    [array addObject:[self createMockItem:url]];
  }
  return [NSArray arrayWithArray:array];
}

@end
