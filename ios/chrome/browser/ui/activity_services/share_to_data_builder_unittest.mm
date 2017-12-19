// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/share_to_data_builder.h"

#include <memory>

#include "base/macros.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/snapshots/snapshot_cache.h"
#import "ios/chrome/browser/snapshots/snapshot_cache_factory.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"
#import "ios/testing/ocmock_complex_type_helper.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "ui/base/test/ios/ui_image_test_utils.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char kExpectedUrl[] = "http://www.testurl.net/";
const char kExpectedTitle[] = "title";

// Returns an image with given |size|, |color| and |scale|.
// TODO(crbug.com/795669): UIImageWithSizeAndSolidColor does not supports
// custom scale, always using 1x; remove this method once this has been
// fixed.
UIImage* UIImageWithSizeAndSolidColorAndScale(const CGSize& size,
                                              UIColor* color,
                                              CGFloat scale) {
  UIGraphicsBeginImageContextWithOptions(size, /*opaque=*/YES, scale);
  CGContext* context = UIGraphicsGetCurrentContext();
  CGContextSetFillColorWithColor(context, [color CGColor]);
  CGContextFillRect(context, CGRect{CGPointZero, size});
  UIImage* image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  return image;
}
}  // namespace

@interface ShareToDataBuilderTestTabMock : OCMockComplexTypeHelper {
  std::unique_ptr<web::TestWebState> _webState;
}

@property(nonatomic, readonly) web::WebState* webState;

@end

@implementation ShareToDataBuilderTestTabMock

- (web::WebState*)webState {
  return _webState.get();
}

- (instancetype)initWithWebState:(std::unique_ptr<web::TestWebState>)webState {
  id representedObject = [OCMockObject niceMockForClass:[Tab class]];
  if ((self = [super initWithRepresentedObject:representedObject])) {
    _webState = std::move(webState);
  }
  return self;
}

- (void)close {
  _webState.reset();
}

@end

class ShareToDataBuilderTest : public PlatformTest {
 public:
  ShareToDataBuilderTest() {
    // Configure a testing factory for SnapshotCache factory to enable the
    // generation of snapshots.
    TestChromeBrowserState::Builder builder;
    builder.AddTestingFactory(SnapshotCacheFactory::GetInstance(),
                              SnapshotCacheFactory::GetDefaultFactory());
    chrome_browser_state_ = builder.Build();

    auto navigation_manager = std::make_unique<web::TestNavigationManager>();
    navigation_manager->AddItem(GURL(kExpectedUrl), ui::PAGE_TRANSITION_TYPED);
    navigation_manager->SetLastCommittedItem(navigation_manager->GetItemAtIndex(
        navigation_manager->GetLastCommittedItemIndex()));
    navigation_manager->GetLastCommittedItem()->SetTitle(
        base::UTF8ToUTF16(kExpectedTitle));

    auto web_state = std::make_unique<web::TestWebState>();
    web_state->SetNavigationManager(std::move(navigation_manager));
    web_state->SetBrowserState(chrome_browser_state_.get());
    web_state->SetVisibleURL(GURL(kExpectedUrl));

    // Set a view to the WebState to allow generating a snapshot.
    CGRect frame = {CGPointZero, CGSizeMake(300, 400)};
    UIView* view = [[UIView alloc] initWithFrame:frame];
    view.backgroundColor = [UIColor blueColor];
    web_state->SetView(view);

    // Attach SnapshotTabHelper and TabIdTabHelper to the WebState to allow
    // generation of the Snapshot.
    TabIdTabHelper::CreateForWebState(web_state.get());
    SnapshotTabHelper::CreateForWebState(web_state.get());

    tab_ = [[ShareToDataBuilderTestTabMock alloc]
        initWithWebState:std::move(web_state)];
    OCMockObject* tab_mock = static_cast<OCMockObject*>(tab_);

    ios::ChromeBrowserState* ptr = chrome_browser_state_.get();
    NSString* expected_title = base::SysUTF8ToNSString(kExpectedTitle);
    [[[tab_mock stub] andReturnValue:OCMOCK_VALUE(ptr)] browserState];
    [[[tab_mock stub] andReturn:expected_title] title];
  }

  void TearDown() override {
    [tab_ close];
    tab_ = nil;
    PlatformTest::TearDown();
  }

  Tab* tab() { return static_cast<Tab*>(tab_); }

  ShareToDataBuilderTestTabMock* tab_mock() { return tab_; }

  // Returns the expected thumbnail for the tab.
  UIImage* GetExpectedThumbnail(const CGSize& thumbnail_size) {
    return UIImageWithSizeAndSolidColorAndScale(
        thumbnail_size, [UIColor blueColor],
        [SnapshotCacheFactory::GetForBrowserState(chrome_browser_state_.get())
            snapshotScaleForDevice]);
  }

 private:
  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<ios::ChromeBrowserState> chrome_browser_state_;
  ShareToDataBuilderTestTabMock* tab_;

  DISALLOW_COPY_AND_ASSIGN(ShareToDataBuilderTest);
};

// Verifies that ShareToData is constructed properly for a given Tab when there
// is a URL provided for share extensions.
TEST_F(ShareToDataBuilderTest, TestSharePageCommandHandlingNpShareUrl) {
  const char* kExpectedShareUrl = "http://www.testurl.com/";
  ShareToData* actual_data =
      activity_services::ShareToDataForTab(tab(), GURL(kExpectedShareUrl));

  ASSERT_TRUE(actual_data);
  EXPECT_EQ(kExpectedShareUrl, actual_data.shareURL);
  EXPECT_EQ(kExpectedUrl, actual_data.passwordManagerURL);
  EXPECT_NSEQ(base::SysUTF8ToNSString(kExpectedTitle), actual_data.title);
  EXPECT_TRUE(actual_data.isOriginalTitle);
  EXPECT_FALSE(actual_data.isPagePrintable);

  const CGSize size = CGSizeMake(40, 40);
  EXPECT_TRUE(ui::test::uiimage_utils::UIImagesAreEqual(
      actual_data.thumbnailGenerator(size), GetExpectedThumbnail(size)));
}

// Verifies that ShareToData is constructed properly for a given Tab when the
// URL designated for share extensions is empty.
TEST_F(ShareToDataBuilderTest, TestSharePageCommandHandlingNoShareUrl) {
  ShareToData* actual_data =
      activity_services::ShareToDataForTab(tab(), GURL());

  ASSERT_TRUE(actual_data);
  EXPECT_EQ(kExpectedUrl, actual_data.shareURL);
  EXPECT_EQ(kExpectedUrl, actual_data.passwordManagerURL);
  EXPECT_NSEQ(base::SysUTF8ToNSString(kExpectedTitle), actual_data.title);
  EXPECT_TRUE(actual_data.isOriginalTitle);
  EXPECT_FALSE(actual_data.isPagePrintable);

  const CGSize size = CGSizeMake(40, 40);
  EXPECT_TRUE(ui::test::uiimage_utils::UIImagesAreEqual(
      actual_data.thumbnailGenerator(size), GetExpectedThumbnail(size)));
}

// Verifies that |ShareToDataForTab()| returns nil if the Tab is in the process
// of being closed.
TEST_F(ShareToDataBuilderTest, TestReturnsNilWhenClosing) {
  [tab_mock() close];

  EXPECT_EQ(nil, activity_services::ShareToDataForTab(tab(), GURL()));
}
