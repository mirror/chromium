// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/share_to_data_builder.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data.h"
#include "ios/web/public/navigation_item.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#import "ios/web/web_state/web_state_impl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "ui/base/test/ios/ui_image_test_utils.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Verifies that ShareToData is constructed properly for a given Tab and
// WebState.
TEST(ShareToDataBuilderTest, TestSharePageCommandHandling) {
  GURL expected_url("http://www.testurl.net");
  NSString* expected_title = @"title";

  // Set up the TestWebState.
  std::unique_ptr<web::NavigationItem> item = web::NavigationItem::Create();
  item->SetURL(expected_url);
  item->SetTitle(base::SysNSStringToUTF16(expected_title));

  std::unique_ptr<web::TestNavigationManager> navigation_manager =
      base::MakeUnique<web::TestNavigationManager>();
  navigation_manager->SetLastCommittedItem(item.get());

  web::TestWebState web_state;
  web_state.SetCurrentURL(expected_url);
  web_state.SetNavigationManager(std::move(navigation_manager));
  web_state.SetTitle(base::SysNSStringToUTF16(expected_title));

  // Set up the mock Tab.
  TestChromeBrowserState::Builder test_cbs_builder;
  std::unique_ptr<ios::ChromeBrowserState> chrome_browser_state =
      test_cbs_builder.Build();
  ios::ChromeBrowserState* ptr = chrome_browser_state.get();
  UIImage* tab_snapshot = ui::test::uiimage_utils::UIImageWithSizeAndSolidColor(
      CGSizeMake(300, 400), [UIColor blueColor]);

  OCMockObject* tab = [OCMockObject mockForClass:[Tab class]];
  [[[tab stub] andReturnValue:OCMOCK_VALUE(ptr)] browserState];
  [[[tab stub] andReturn:nil] viewForPrinting];
  [[[tab stub] andReturn:tab_snapshot] generateSnapshotWithOverlay:NO
                                                  visibleFrameOnly:YES];

  ShareToData* actual_data =
      activity_services::ShareToDataForTab(static_cast<Tab*>(tab), &web_state);
  ASSERT_TRUE(actual_data);
  EXPECT_EQ(expected_url, actual_data.url);
  EXPECT_NSEQ(expected_title, actual_data.title);
  EXPECT_TRUE(actual_data.isOriginalTitle);
  EXPECT_FALSE(actual_data.isPagePrintable);

  CGSize size = CGSizeMake(40, 40);
  EXPECT_TRUE(ui::test::uiimage_utils::UIImagesAreEqual(
      actual_data.thumbnailGenerator(size),
      ui::test::uiimage_utils::UIImageWithSizeAndSolidColor(
          size, [UIColor blueColor])));
}

// Verifies that |ShareToDataForTab()| returns nil if the Tab is in the process
// of being closed.
TEST(ShareToDataBuilderTest, TestReturnsNilWhenClosing) {
  OCMockObject* tab = [OCMockObject mockForClass:[Tab class]];
  EXPECT_EQ(nil, activity_services::ShareToDataForTab(static_cast<Tab*>(tab),
                                                      nullptr));
}
