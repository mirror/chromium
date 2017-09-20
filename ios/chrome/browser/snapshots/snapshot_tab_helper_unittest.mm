// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"

#include "base/mac/bind_objc_block.h"
#include "base/test/scoped_task_environment.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/snapshots/snapshot_cache.h"
#import "ios/chrome/browser/snapshots/snapshot_provider.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Concrete SnapshotProvider for testing.
@interface TestSnapshotProvider : NSObject<SnapshotProvider>
// Used to check if the -provideSnapshot method was called.
@property(nonatomic, assign) BOOL methodCalled;
@end

@implementation TestSnapshotProvider
@synthesize methodCalled = _methodCalled;
- (void)provideSnapshot:(SnapshotProvidingBlock)snapshotProvidingBlock {
  self.methodCalled = YES;
  snapshotProvidingBlock([[UIImage alloc] init]);
}
@end

namespace {

// Test fixture for SnapshotTabHelper class.
class SnapshotTabHelperTest : public PlatformTest {
 protected:
  SnapshotTabHelperTest() {
    web_state_ = base::MakeUnique<web::TestWebState>();
    browser_state_ = TestChromeBrowserState::Builder().Build();
    web_state_->SetBrowserState(browser_state_.get());
    snapshot_cache_ = OCMClassMock([SnapshotCache class]);
    TabIdTabHelper::CreateForWebState(web_state_.get());
    tab_id_ = TabIdTabHelper::FromWebState(web_state_.get())->tab_id();
    SnapshotTabHelper::CreateForWebState(web_state_.get(), snapshot_cache_);
    tab_helper_ = SnapshotTabHelper::FromWebState(web_state_.get());
    snapshot_provider_ = [[TestSnapshotProvider alloc] init];
  }
  ~SnapshotTabHelperTest() override {}

  base::test::ScopedTaskEnvironment environment_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  std::unique_ptr<web::TestWebState> web_state_;
  NSString* tab_id_;
  SnapshotTabHelper* tab_helper_;
  id snapshot_cache_;
  TestSnapshotProvider* snapshot_provider_;
};

}  // namespace

// Tests that TakeSnapshot updates the cache.
TEST_F(SnapshotTabHelperTest, TakeSnapshot) {
  [[snapshot_cache_ expect] setImage:[OCMArg any] withSessionID:tab_id_];
  tab_helper_->TakeSnapshot();
  EXPECT_OCMOCK_VERIFY(snapshot_cache_);
}

// Tests that the provider is used when URL has Chrome scheme.
TEST_F(SnapshotTabHelperTest, UsesProvider) {
  snapshot_provider_.methodCalled = NO;
  web_state_->SetCurrentURL(GURL(kChromeUINewTabURL));
  tab_helper_->SetSnapshotProvider(snapshot_provider_);
  [[snapshot_cache_ expect] setImage:[OCMArg any] withSessionID:tab_id_];
  tab_helper_->TakeSnapshot();
  EXPECT_OCMOCK_VERIFY(snapshot_cache_);
  testing::WaitUntilConditionOrTimeout(
      testing::kWaitForSnapshotCompletionTimeout, ^{
        return snapshot_provider_.methodCalled;
      });
  EXPECT_TRUE(snapshot_provider_.methodCalled);
}

// Tests that the provider is not used when URL is not Chrome scheme.
TEST_F(SnapshotTabHelperTest, DoesNotUseProvider) {
  snapshot_provider_.methodCalled = NO;
  tab_helper_->SetSnapshotProvider(snapshot_provider_);
  [[snapshot_cache_ expect] setImage:[OCMArg any] withSessionID:tab_id_];
  tab_helper_->TakeSnapshot();
  EXPECT_OCMOCK_VERIFY(snapshot_cache_);
  testing::WaitUntilConditionOrTimeout(
      testing::kWaitForSnapshotCompletionTimeout, ^{
        return snapshot_provider_.methodCalled;
      });
  EXPECT_FALSE(snapshot_provider_.methodCalled);
}

// Tests that snapshot is removed from cache on WebStateDestroyed.
TEST_F(SnapshotTabHelperTest, WebStateDestroyed) {
  [[snapshot_cache_ expect] removeImageWithSessionID:tab_id_];
  web_state_.reset();
  EXPECT_OCMOCK_VERIFY(snapshot_cache_);
}
