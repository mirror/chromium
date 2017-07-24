// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/wk_based_navigation_manager_impl.h"

#include <WebKit/WebKit.h>
#include <memory>

#include "base/memory/ptr_util.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/test/fakes/test_browser_state.h"
#import "ios/web/test/fakes/crw_test_back_forward_list.h"
#include "ios/web/test/fakes/test_navigation_manager_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

// Test fixture for WKBasedNavigationManagerImpl.
class WKBasedNavigationManagerTest : public PlatformTest {
 protected:
  WKBasedNavigationManagerTest() {
    manager_ = base::MakeUnique<WKBasedNavigationManagerImpl>();
    mock_web_view_ = OCMClassMock([WKWebView class]);
    mock_wk_list_ = [CRWTestBackForwardList alloc];
    OCMStub([mock_web_view_ backForwardList]).andReturn(mock_wk_list_);
    delegate_.SetWebViewNavigationProxy(mock_web_view_);

    manager_->SetDelegate(&delegate_);
    manager_->SetBrowserState(&browser_state_);
  }

 protected:
  std::unique_ptr<NavigationManagerImpl> manager_;
  CRWTestBackForwardList* mock_wk_list_;
  // WKWebView* mock_web_view_;
  id mock_web_view_;

 private:
  TestBrowserState browser_state_;
  TestNavigationManagerDelegate delegate_;
};

// Tests that GetItemAtIndex() on an empty manager will sync navigation itmes to
// WKBackForwardList using default properties.
TEST_F(WKBasedNavigationManagerTest, SyncAfterItemAtIndex) {
  EXPECT_EQ(0, manager_->GetItemCount());
  EXPECT_EQ(nullptr, manager_->GetItemAtIndex(0));

  [mock_wk_list_ setCurrentURL:@"http://www.0.com"];
  EXPECT_EQ(1, manager_->GetItemCount());

  NavigationItem* item = manager_->GetItemAtIndex(0);
  EXPECT_EQ(GURL("http://www.0.com"), item->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_LINK,
                                           item->GetTransitionType()));
  EXPECT_EQ(UserAgentType::MOBILE, item->GetUserAgentType());
}

// Tests that Referrer is inferred from the previous WKBackForwardListItem.
TEST_F(WKBasedNavigationManagerTest, SyncMultipleItems) {
  [mock_wk_list_
        setCurrentURL:@"http://www.1.com"
         backListURLs:[NSArray arrayWithObjects:@"http://www.0.com", nil]
      forwardListURLs:[NSArray arrayWithObjects:@"http://www.2.com", nil]];
  EXPECT_EQ(3, manager_->GetItemCount());

  // GetLastCommittedItemIndex() should return the out-of-sync value now.
  EXPECT_EQ(-1, manager_->GetLastCommittedItemIndex());

  NavigationItem* item0 = manager_->GetItemAtIndex(0);
  EXPECT_EQ(GURL("http://www.0.com"), item0->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_LINK,
                                           item0->GetTransitionType()));
  EXPECT_EQ(UserAgentType::MOBILE, item0->GetUserAgentType());

  // GetLastCommittedItemIndex() is now synced.
  EXPECT_EQ(1, manager_->GetLastCommittedItemIndex());

  NavigationItem* item1 = manager_->GetItemAtIndex(1);
  EXPECT_EQ(GURL("http://www.1.com"), item1->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_LINK,
                                           item1->GetTransitionType()));
  EXPECT_EQ(UserAgentType::MOBILE, item1->GetUserAgentType());
  EXPECT_EQ(GURL("http://www.0.com"), item1->GetReferrer().url);

  NavigationItem* item2 = manager_->GetItemAtIndex(2);
  EXPECT_EQ(GURL("http://www.2.com"), item2->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_LINK,
                                           item2->GetTransitionType()));
  EXPECT_EQ(UserAgentType::MOBILE, item2->GetUserAgentType());
  EXPECT_EQ(GURL("http://www.1.com"), item2->GetReferrer().url);
}

// Tests that CommitPendingItem() will sync navigation items to
// WKBackForwardList and the pending item NavigationItemImpl will be used.
TEST_F(WKBasedNavigationManagerTest, GetItemAtIndexAfterCommitPending) {
  // Simulate a main frame navigation.
  manager_->AddPendingItem(
      GURL("http://www.0.com"), Referrer(), ui::PAGE_TRANSITION_TYPED,
      web::NavigationInitiationType::USER_INITIATED,
      web::NavigationManager::UserAgentOverrideOption::DESKTOP);

  [mock_wk_list_ setCurrentURL:@"http://www.0.com"];
  manager_->CommitPendingItem();

  EXPECT_EQ(1, manager_->GetItemCount());
  NavigationItem* item = manager_->GetLastCommittedItem();
  EXPECT_EQ(GURL("http://www.0.com"), item->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_TYPED,
                                           item->GetTransitionType()));
  EXPECT_EQ(UserAgentType::DESKTOP, item->GetUserAgentType());

  // Simulate a second main frame navigation.
  manager_->AddPendingItem(
      GURL("http://www.2.com"), Referrer(), ui::PAGE_TRANSITION_TYPED,
      web::NavigationInitiationType::USER_INITIATED,
      web::NavigationManager::UserAgentOverrideOption::DESKTOP);

  // Simulate an iframe navigation between the two main frame navigations.
  [mock_wk_list_
        setCurrentURL:@"http://www.2.com"
         backListURLs:[NSArray arrayWithObjects:@"http://www.0.com",
                                                @"http://www.1.com", nil]
      forwardListURLs:nil];
  manager_->CommitPendingItem();

  EXPECT_EQ(3, manager_->GetItemCount());
  EXPECT_EQ(2, manager_->GetLastCommittedItemIndex());

  NavigationItem* item1 = manager_->GetItemAtIndex(1);
  EXPECT_EQ(GURL("http://www.1.com"), item1->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_LINK,
                                           item1->GetTransitionType()));
  EXPECT_EQ(UserAgentType::MOBILE, item1->GetUserAgentType());
  EXPECT_EQ(GURL("http://www.0.com"), item1->GetReferrer().url);

  NavigationItem* item2 = manager_->GetItemAtIndex(2);
  EXPECT_EQ(GURL("http://www.2.com"), item2->GetURL());
  EXPECT_TRUE(ui::PageTransitionCoreTypeIs(ui::PAGE_TRANSITION_TYPED,
                                           item2->GetTransitionType()));
  EXPECT_EQ(UserAgentType::DESKTOP, item2->GetUserAgentType());
  EXPECT_EQ(GURL(""), item2->GetReferrer().url);
}

// Tests DiscardNonCommittedItems discards both pending and transient items.
TEST_F(WKBasedNavigationManagerTest, DiscardNonCommittedItems) {
  manager_->AddPendingItem(
      GURL("http://www.0.com"), Referrer(), ui::PAGE_TRANSITION_TYPED,
      web::NavigationInitiationType::USER_INITIATED,
      web::NavigationManager::UserAgentOverrideOption::DESKTOP);
  manager_->AddTransientItem(GURL("http://www.1.com"));

  EXPECT_NE(nullptr, manager_->GetPendingItem());
  EXPECT_NE(nullptr, manager_->GetTransientItem());

  manager_->DiscardNonCommittedItems();
  EXPECT_EQ(nullptr, manager_->GetPendingItem());
  EXPECT_EQ(nullptr, manager_->GetTransientItem());
}

// Tests that in the absenece of a transient item, going back is delegated to
// the underlying WKWebView without any sanity checks such as whether any back
// history exists.
TEST_F(WKBasedNavigationManagerTest, GoBackWithoutTransientItem) {
  [(WKWebView*)[mock_web_view_ expect] goBack];
  manager_->GoBack();
  [mock_web_view_ verify];
}

// Tests that going back from a transient item will discard the transient item
// without any navigations on the underlying WKBackForwardList.
TEST_F(WKBasedNavigationManagerTest, GoBackFromTransientItem) {
  manager_->AddPendingItem(
      GURL("http://www.0.com"), Referrer(), ui::PAGE_TRANSITION_TYPED,
      web::NavigationInitiationType::USER_INITIATED,
      web::NavigationManager::UserAgentOverrideOption::DESKTOP);
  manager_->AddTransientItem(GURL("http://www.1.com"));

  [(WKWebView*)[mock_web_view_ reject] goBack];
  manager_->GoBack();
  [mock_web_view_ verify];

  EXPECT_NE(nullptr, manager_->GetPendingItem());
  EXPECT_EQ(nullptr, manager_->GetTransientItem());
}

// Tests that going forward is always delegated to the underlying WKWebView
// without any sanity checks such as whether any forward history exists.
TEST_F(WKBasedNavigationManagerTest, GoForward) {
  [(WKWebView*)[mock_web_view_ expect] goForward];
  manager_->GoForward();
  [mock_web_view_ verify];
}

}  // namespace web
