// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/download/pass_kit_tab_helper.h"

#include <memory>

#import <PassKit/PassKit.h>

#import "ios/chrome/browser/download/pass_kit_tab_helper_delegate.h"
#include "ios/chrome/browser/download/pass_kit_test_util.h"
#import "ios/web/public/test/fakes/fake_download_task.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// PassKitTabHelperDelegate which collects all passes into |passes| array.
@interface FakePassKitTabHelperDelegate : NSObject<PassKitTabHelperDelegate> {
  web::WebState* _webState;
  NSMutableArray* _passes;
}
// All passes presented by PassKitTabHelper. nil passes are represented with
// NSNull objects.
@property(nonatomic, readonly) NSArray<PKPass*>* passes;
@end

@implementation FakePassKitTabHelperDelegate

- (instancetype)initWithWebState:(web::WebState*)webState {
  self = [super init];
  if (self) {
    _webState = webState;
    _passes = [NSMutableArray array];
  }
  return self;
}

- (NSArray<PKPass*>*)passes {
  return [_passes copy];
}

- (void)passKitTabHelper:(nonnull PassKitTabHelper*)tabHelper
    presentDialogForPass:(nullable PKPass*)pass
                webState:(nonnull web::WebState*)webState {
  DCHECK_EQ(_webState, webState);
  DCHECK_EQ(PassKitTabHelper::FromWebState(_webState), tabHelper);
  [_passes addObject:pass ? pass : [NSNull null]];
}

@end

namespace {
char kUrl[] = "https://test.test/";
char kMimeType[] = "application/vnd.apple.pkpass";

// Used as no-op callback.
void DoNothing(int) {}
}  // namespace

// Test fixture for testing PassKitTabHelper class.
class PassKitTabHelperTest : public PlatformTest {
 protected:
  PassKitTabHelperTest()
      : delegate_([[FakePassKitTabHelperDelegate alloc]
            initWithWebState:&web_state_]) {
    PassKitTabHelper::CreateForWebState(&web_state_, delegate_);
  }

  PassKitTabHelper* tab_helper() {
    return PassKitTabHelper::FromWebState(&web_state_);
  }

  std::unique_ptr<web::FakeDownloadTask> create_task() {
    return std::make_unique<web::FakeDownloadTask>(GURL(kUrl), kMimeType);
  }

  web::TestWebState web_state_;
  FakePassKitTabHelperDelegate* delegate_;
};

// Tests downloading empty pkpass file.
TEST_F(PassKitTabHelperTest, EmptyFile) {
  auto task = std::make_unique<web::FakeDownloadTask>(GURL(kUrl), kMimeType);
  web::FakeDownloadTask* task_ptr = task.get();
  tab_helper()->Download(std::move(task));
  task_ptr->SetDone(true);
  EXPECT_EQ(1U, delegate_.passes.count);
  EXPECT_TRUE([delegate_.passes.firstObject isKindOfClass:[NSNull class]]);
}

// Tests downloading a valid pkpass file.
TEST_F(PassKitTabHelperTest, ValidPassKitFile) {
  auto task = std::make_unique<web::FakeDownloadTask>(GURL(kUrl), kMimeType);
  web::FakeDownloadTask* task_ptr = task.get();
  tab_helper()->Download(std::move(task));

  std::string pass_data = testing::GetTestPass();
  auto buffer = base::MakeRefCounted<net::IOBuffer>(pass_data.size());
  memcpy(buffer->data(), pass_data.c_str(), pass_data.size());
  // Writing to URLFetcherStringWriter, which is used by PassKitTabHelper is
  // synchronous, so it's ok to ignore Write's completion callback.
  task_ptr->GetResponseWriter()->Write(buffer.get(), pass_data.size(),
                                       base::BindRepeating(&DoNothing));
  task_ptr->SetDone(true);

  EXPECT_EQ(1U, delegate_.passes.count);
  PKPass* pass = delegate_.passes.firstObject;
  EXPECT_TRUE([pass isKindOfClass:[PKPass class]]);
  EXPECT_EQ(PKPassTypeBarcode, pass.passType);
  EXPECT_NSEQ(@"pass.com.apple.devpubs.example", pass.passTypeIdentifier);
  EXPECT_NSEQ(@"Toy Town", pass.organizationName);
}
