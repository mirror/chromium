// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NAVIGATION_CRW_NAVIGATION_ITEM_HOLDER_H_
#define IOS_WEB_NAVIGATION_CRW_NAVIGATION_ITEM_HOLDER_H_

#include <WebKit/WebKit.h>
#include <memory>

#import "ios/web/navigation/navigation_item_impl.h"

// An NSObject wrapper for an std::unique_ptr<NavigationItemImpl>. This object
// is used to associated a NavigationItemImpl to a WKBackForwardListItem to
// store additional navigation states needed by the embedder.
@interface CRWNavigationItemHolder : NSObject

// Returns the CRWNavigationItemHolder object associated with |item|. Creates an
// empty holder if none currently exists for |item|.
+ (nonnull instancetype)holderForBackForwardListItem:
    (nonnull WKBackForwardListItem*)item;

// Returns the NavigationItemImpl stored in this instance.
@property(nullable, nonatomic, readonly)
    web::NavigationItemImpl* navigationItem;

// Stores a NavigationItemImpl object in this instance.
- (void)setNavigationItem:
    (std::unique_ptr<web::NavigationItemImpl>)navigationItem;

@end

#endif  // IOS_WEB_NAVIGATION_CRW_NAVIGATION_ITEM_HOLDER_H_
