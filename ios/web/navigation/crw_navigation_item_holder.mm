// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_navigation_item_holder.h"

#import <objc/runtime.h>

// The address of this static variable is used to set and get the associated
// NavigationItemImpl object from a WKBackForwardListItem.
const void* kNavigationItemKey = &kNavigationItemKey;

@implementation CRWNavigationItemHolder {
  std::unique_ptr<web::NavigationItemImpl> _navigationItem;
}

+ (instancetype)holderForBackForwardListItem:(WKBackForwardListItem*)item {
  if (!item) {
    return nil;
  }

  CRWNavigationItemHolder* holder =
      objc_getAssociatedObject(item, &kNavigationItemKey);
  if (!holder) {
    holder = [[CRWNavigationItemHolder alloc] init];
    objc_setAssociatedObject(item, &kNavigationItemKey, holder,
                             OBJC_ASSOCIATION_RETAIN);
  }
  return holder;
}

- (web::NavigationItemImpl*)navigationItem {
  return _navigationItem.get();
}

- (void)setNavigationItem:
    (std::unique_ptr<web::NavigationItemImpl>)navigationItem {
  _navigationItem = std::move(navigationItem);
}

@end
