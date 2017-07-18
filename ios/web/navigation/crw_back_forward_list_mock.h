// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_CRW_BACK_FORWARD_LIST_MOCK_H_
#define IOS_WEB_NAVIGATION_CRW_BACK_FORWARD_LIST_MOCK_H_

#import <Foundation/Foundation.h>

@class WKBackForwardListItem;

// A CRWBackForwardListMock can be used to mock out WKBackForwardList in tests.
@interface CRWBackForwardListMock : NSObject
@property NSArray<WKBackForwardListItem*>* backList;
@property NSArray<WKBackForwardListItem*>* forwardList;
@property WKBackForwardListItem* currentItem;
- (WKBackForwardListItem*)itemAtIndex:(NSInteger)index;

- (void)setCurrentUrl:(NSString*)currentItemUrl;
- (void)setCurrentUrl:(NSString*)currentItemUrl
         backListUrls:(NSArray<NSString*>*)backListUrls
      forwardListUrls:(NSArray<NSString*>*)forwardListUrls;
@end

#endif  // IOS_WEB_NAVIGATION_CRW_BACK_FORWARD_LIST_MOCK_H_
