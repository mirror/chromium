// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_navigation_item_holder.h"

@implementation CRWNavigationItemHolder {
  std::unique_ptr<web::NavigationItemImpl> _navigationItem;
}

- (instancetype)initWithNavigationItem:
    (std::unique_ptr<web::NavigationItemImpl>)navItem {
  self = [super init];
  if (self) {
    _navigationItem = std::move(navItem);
  }
  return self;
}

- (web::NavigationItemImpl*)navigationItem {
  return _navigationItem.get();
}

@end
