// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_navigation_item_holder.h"

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for CRWNavigationItemHolder.
class CRWNavigationItemHolderTest : public PlatformTest {};

TEST_F(CRWNavigationItemHolderTest, NewHolderCreatedAutomatically) {
  WKBackForwardListItem* item = OCMClassMock([WKBackForwardListItem class]);
  CRWNavigationItemHolder* holder =
      [CRWNavigationItemHolder holderForBackForwardListItem:item];
  ASSERT_NSNE(holder, nil);
  EXPECT_EQ(holder.navigationItem, nullptr);
}

// Tests that stored NavigationItemImpl can be retrieved.
TEST_F(CRWNavigationItemHolderTest, SetNavigationItem) {
  GURL URL("http://www.0.com");
  auto navigation_item = base::MakeUnique<web::NavigationItemImpl>();
  navigation_item->SetURL(URL);

  WKBackForwardListItem* item = OCMClassMock([WKBackForwardListItem class]);
  [[CRWNavigationItemHolder holderForBackForwardListItem:item]
      setNavigationItem:std::move(navigation_item)];

  CRWNavigationItemHolder* holder =
      [CRWNavigationItemHolder holderForBackForwardListItem:item];

  ASSERT_NSNE(holder, nil);
  EXPECT_EQ(URL, holder.navigationItem->GetURL());
}

// Tests that each WKBackForwardListItem has its unique CRWNavigationItemHolder.
TEST_F(CRWNavigationItemHolderTest, OneHolderPerWKItem) {
  GURL URL1("http://www.1.com");
  auto navigation_item1 = base::MakeUnique<web::NavigationItemImpl>();
  navigation_item1->SetURL(URL1);
  WKBackForwardListItem* item1 = OCMClassMock([WKBackForwardListItem class]);
  [[CRWNavigationItemHolder holderForBackForwardListItem:item1]
      setNavigationItem:std::move(navigation_item1)];

  GURL URL2("http://www.2.com");
  auto navigation_item2 = base::MakeUnique<web::NavigationItemImpl>();
  navigation_item2->SetURL(URL2);
  WKBackForwardListItem* item2 = OCMClassMock([WKBackForwardListItem class]);
  [[CRWNavigationItemHolder holderForBackForwardListItem:item2]
      setNavigationItem:std::move(navigation_item2)];

  ASSERT_NSNE(item1, item2);

  CRWNavigationItemHolder* holder1 =
      [CRWNavigationItemHolder holderForBackForwardListItem:item1];
  CRWNavigationItemHolder* holder2 =
      [CRWNavigationItemHolder holderForBackForwardListItem:item2];

  EXPECT_NSNE(holder1, holder2);
  EXPECT_EQ(URL1, holder1.navigationItem->GetURL());
  EXPECT_EQ(URL2, holder2.navigationItem->GetURL());
}
