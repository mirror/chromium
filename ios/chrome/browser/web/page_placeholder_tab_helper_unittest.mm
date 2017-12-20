// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/page_placeholder_tab_helper.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/test/ios/wait_util.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/snapshots/snapshot_cache_factory.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for PagePlaceholderTabHelper class.
class PagePlaceholderTabHelperTest : public PlatformTest {
 protected:
  PagePlaceholderTabHelperTest() {
    // Create a SnapshotCache instance as the SnapshotGenerator first lookup
    // in the cache before generating the snapshot (thus the test would fail
    // if no cache existed).
    TestChromeBrowserState::Builder builder;
    builder.AddTestingFactory(SnapshotCacheFactory::GetInstance(),
                              SnapshotCacheFactory::GetDefaultFactory());
    browser_state_ = builder.Build();
    web_state_.SetBrowserState(browser_state_.get());

    // PagePlaceholderTabHelper uses SnapshotTabHelper to generate the snapshot
    // which in turn uses TabIdTabHelper to get a unique identifier for the
    // WebState.
    TabIdTabHelper::CreateForWebState(&web_state_);
    SnapshotTabHelper::CreateForWebState(&web_state_);

    PagePlaceholderTabHelper::CreateForWebState(&web_state_);
  }

  PagePlaceholderTabHelper* tab_helper() {
    return PagePlaceholderTabHelper::FromWebState(&web_state_);
  }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<ios::ChromeBrowserState> browser_state_;
  web::TestWebState web_state_;
};

// Tests that placeholder is not shown after DidStartNavigation if it was not
// requested.
TEST_F(PagePlaceholderTabHelperTest, NotShown) {
  ASSERT_FALSE(tab_helper()->displaying_placeholder());
  ASSERT_FALSE(tab_helper()->will_add_placeholder_for_next_navigation());
  web_state_.OnNavigationStarted(nullptr);
  EXPECT_FALSE(tab_helper()->displaying_placeholder());
  EXPECT_FALSE(tab_helper()->will_add_placeholder_for_next_navigation());
}

// Tests that placeholder is shown between DidStartNavigation/PageLoaded
// WebStateObserver callbacks.
TEST_F(PagePlaceholderTabHelperTest, Shown) {
  ASSERT_FALSE(tab_helper()->will_add_placeholder_for_next_navigation());
  tab_helper()->AddPlaceholderForNextNavigation();
  ASSERT_FALSE(tab_helper()->displaying_placeholder());
  EXPECT_TRUE(tab_helper()->will_add_placeholder_for_next_navigation());

  web_state_.OnNavigationStarted(nullptr);
  EXPECT_TRUE(tab_helper()->displaying_placeholder());
  EXPECT_FALSE(tab_helper()->will_add_placeholder_for_next_navigation());

  web_state_.OnPageLoaded(web::PageLoadCompletionStatus::SUCCESS);
  EXPECT_FALSE(tab_helper()->displaying_placeholder());
  EXPECT_FALSE(tab_helper()->will_add_placeholder_for_next_navigation());
}
