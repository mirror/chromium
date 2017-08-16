// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/share_to_data_builder.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data.h"
#include "ios/testing/ocmock_complex_type_helper.h"
#import "ios/web/public/navigation_item.h"
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

@interface ShareToDataBuilderTestTabMock : OCMockComplexTypeHelper {
  std::unique_ptr<web::TestWebState> _webState;
}

@property(nonatomic, readonly) web::WebState* webState;

- (web::TestWebState*)createWebState;

@end

@implementation ShareToDataBuilderTestTabMock
- (web::WebState*)webState {
  return _webState.get();
}

- (web::TestWebState*)createWebState {
  _webState = base::MakeUnique<web::TestWebState>();
  _webState->SetNavigationManager(
      base::MakeUnique<web::TestNavigationManager>());
  return _webState.get();
}
@end

// Verifies that ShareToData is constructed properly for a given Tab.
TEST(ShareToDataBuilderTest, TestSharePageCommandHandling) {
  GURL expected_url("http://www.testurl.net");
  NSString* expected_title = @"title";

  // Set up the mock Tab.
  TestChromeBrowserState::Builder test_cbs_builder;
  std::unique_ptr<ios::ChromeBrowserState> chrome_browser_state =
      test_cbs_builder.Build();
  ios::ChromeBrowserState* ptr = chrome_browser_state.get();
  UIImage* tab_snapshot = ui::test::uiimage_utils::UIImageWithSizeAndSolidColor(
      CGSizeMake(300, 400), [UIColor blueColor]);

  ShareToDataBuilderTestTabMock* tab = [[ShareToDataBuilderTestTabMock alloc]
      initWithRepresentedObject:[OCMockObject mockForClass:[Tab class]]];
  id tab_mock = tab;
  [[[tab_mock stub] andReturnValue:OCMOCK_VALUE(ptr)] browserState];
  [[[tab_mock stub] andReturn:nil] viewForPrinting];
  [[[tab_mock stub] andReturn:tab_snapshot] generateSnapshotWithOverlay:NO
                                                       visibleFrameOnly:YES];

  // Set up the TestWebState.
  web::TestWebState* web_state = [tab createWebState];
  web_state->SetCurrentURL(expected_url);
  web_state->SetTitle(base::SysNSStringToUTF16(expected_title));

  std::unique_ptr<web::NavigationItem> item = web::NavigationItem::Create();
  item->SetURL(expected_url);
  item->SetTitle(base::SysNSStringToUTF16(expected_title));

  web::TestNavigationManager* navigation_manager =
      static_cast<web::TestNavigationManager*>(
          web_state->GetNavigationManager());
  navigation_manager->SetLastCommittedItem(item.get());

  // Verify that the actual data matches the expected values.
  ShareToData* actual_data =
      activity_services::ShareToDataForTab(static_cast<Tab*>(tab));
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
  ShareToDataBuilderTestTabMock* tab = [[ShareToDataBuilderTestTabMock alloc]
      initWithRepresentedObject:[OCMockObject mockForClass:[Tab class]]];
  EXPECT_EQ(nil, activity_services::ShareToDataForTab(static_cast<Tab*>(tab)));
}
