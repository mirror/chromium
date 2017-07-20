// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_CRW_BACK_FORWARD_LIST_MOCK_H_
#define IOS_WEB_NAVIGATION_CRW_BACK_FORWARD_LIST_MOCK_H_

#import <Foundation/Foundation.h>

@class WKBackForwardListItem;

// A CRWBackForwardListMock can be used to mock out WKBackForwardList in tests.
@interface CRWBackForwardListMock : NSObject

// WKBackForwardList interface
@property(nonatomic, readonly, copy) NSArray<WKBackForwardListItem*>* backList;
@property(nonatomic, readonly, copy)
    NSArray<WKBackForwardListItem*>* forwardList;
@property(nullable, nonatomic, readonly, strong)
    WKBackForwardListItem* currentItem;
- (nullable WKBackForwardListItem*)itemAtIndex:(NSInteger)index;

// Reset this instance to simulate a session that contains a single entry for
// |currentItemURL|, with no back or forward history.
- (void)setCurrentURL:(NSString*)currentItemURL;

// Reset this instance to simulate a session with the current entry at
// |currentItemURL|, and back and forward history entries as specified in
// |backListURLs| and |forwardListURLs|.
- (void)setCurrentURL:(NSString*)currentItemURL
         backListURLs:(NSArray<NSString*>*)backListURLs
      forwardListURLs:(NSArray<NSString*>*)forwardListURLs;
@end

#endif  // IOS_WEB_NAVIGATION_CRW_BACK_FORWARD_LIST_MOCK_H_
