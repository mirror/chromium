// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_controller_impl.h"

#include <memory>

#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/testing/wait_util.h"
#include "ios/web/public/test/fakes/fake_download_controller_observer.h"
#include "ios/web/public/test/fakes/test_browser_state.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;

namespace web {

namespace {
const char kContentDisposition[] = "attachment; filename=file.test";
const char kMimeType[] = "application/pdf";
}  //  namespace

// Test fixture for testing DownloadControllerImpl class.
class DownloadControllerImplTest : public PlatformTest {
 protected:
  DownloadControllerImplTest()
      : browser_state_(base::MakeUnique<TestBrowserState>()),
        observer_(download_controller()) {
    web_state_.SetBrowserState(browser_state_.get());
  }
  ~DownloadControllerImplTest() override {
    // Destroy DownloadController before FakeDownloadControllerObserver to test
    // observer removal inside OnDownloadControllerDestroyed() callback.
    browser_state_.reset();
  }

  DownloadController* download_controller() {
    return DownloadController::FromBrowserState(browser_state_.get());
  }

  TestWebThreadBundle thread_bundle_;
  TestWebState web_state_;
  std::unique_ptr<BrowserState> browser_state_;
  FakeDownloadControllerObserver observer_;
};

// Tests that DownloadController::FromBrowserState returns the same object for
// each call.
TEST_F(DownloadControllerImplTest, FromBrowserState) {
  DownloadController* controller = download_controller();
  ASSERT_TRUE(controller);
  ASSERT_EQ(controller, download_controller());
}

// Tests that DownloadController::CreateDownloadTask calls
// DownloadControllerObserver::OnDownloadCreated.
TEST_F(DownloadControllerImplTest, OnDownloadCreated) {
  NSString* identifier = [NSUUID UUID].UUIDString;
  GURL url("https://download.test");
  scoped_refptr<DownloadTask> task = download_controller()->CreateDownloadTask(
      &web_state_, identifier, url, kContentDisposition, -1, kMimeType);
  ASSERT_TRUE(task);

  ASSERT_EQ(1U, observer_.alive_download_tasks().size());
  EXPECT_EQ(task.get(), observer_.alive_download_tasks()[0].second.get());
  EXPECT_EQ(&web_state_, observer_.alive_download_tasks()[0].first);
  EXPECT_NSEQ(identifier, task->GetIndentifier());
  EXPECT_EQ(url, task->GetOriginalUrl());
  EXPECT_FALSE(task->IsDone());
  EXPECT_EQ(0, task->GetErrorCode());
  EXPECT_EQ(-1, task->GetTotalBytes());
  EXPECT_EQ(-1, task->GetPercentComplete());
  EXPECT_EQ(kContentDisposition, task->GetContentDisposition());
  EXPECT_EQ(kMimeType, task->GetMimeType());
  EXPECT_EQ("file.test", base::UTF16ToUTF8(task->GetSuggestedFilename()));
}

// Tests that starting download task calls
// DownloadControllerObserver::OnDownloadUpdated. The test will attempt to make
// a real connection to unexisting server but the server availablity will not
// affect the test.
TEST_F(DownloadControllerImplTest, OnDownloadUpdated) {
  NSString* identifier = [NSUUID UUID].UUIDString;
  scoped_refptr<DownloadTask> task = download_controller()->CreateDownloadTask(
      &web_state_, identifier, GURL("https://download.test/"),
      kContentDisposition, -1, kMimeType);
  ASSERT_TRUE(task);
  ASSERT_TRUE(observer_.destroyed_download_tasks().empty());
  ASSERT_TRUE(observer_.updated_download_tasks().empty());
  ASSERT_EQ(1U, observer_.alive_download_tasks().size());

  task->SetResponseWriter(base::MakeUnique<net::URLFetcherStringWriter>());
  task->Start();
  ASSERT_TRUE(WaitUntilConditionOrTimeout(testing::kWaitForPageLoadTimeout, ^{
    base::RunLoop().RunUntilIdle();
    return !observer_.updated_download_tasks().empty();
  }));

  ASSERT_EQ(task, observer_.updated_download_tasks()[0].second);
}

// Tests that destructing download task calls
// DownloadControllerObserver::OnDownloadDestroyed.
TEST_F(DownloadControllerImplTest, OnDownloadDestroyed) {
  scoped_refptr<DownloadTask> task = download_controller()->CreateDownloadTask(
      &web_state_, [NSUUID UUID].UUIDString, GURL("https://download.test"),
      kContentDisposition, -1, kMimeType);
  ASSERT_TRUE(task);
  ASSERT_TRUE(observer_.destroyed_download_tasks().empty());
  ASSERT_EQ(1U, observer_.alive_download_tasks().size());

  observer_.RemoveDownloadTask(task.get());
  DownloadTask* task_ptr = task.get();
  task = nullptr;
  ASSERT_TRUE(observer_.alive_download_tasks().empty());
  ASSERT_EQ(1U, observer_.destroyed_download_tasks().size());
  EXPECT_EQ(task_ptr, observer_.destroyed_download_tasks()[0].second);
  EXPECT_EQ(&web_state_, observer_.destroyed_download_tasks()[0].first);
}

}  // namespace web
