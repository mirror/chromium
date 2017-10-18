// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/download/download_controller_impl.h"

#include "base/strings/utf_string_conversions.h"
#include "ios/web/public/test/fakes/fake_download_controller_observer.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

namespace {
const char kContentDisposition[] = "attachment; filename=file.test";
const char kMimeType[] = "application/pdf";
}  //  namespace

// Test fixture for testing DownloadControllerImpl class.
class DownloadControllerImplTest : public WebTest {
 protected:
  DownloadControllerImplTest() {
    download_controller()->AddObserver(&observer_);
  }
  ~DownloadControllerImplTest() override {
    download_controller()->RemoveObserver(&observer_);
  }

  DownloadController* download_controller() {
    return DownloadController::FromBrowserState(GetBrowserState());
  }

  TestWebState web_state_;
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
  download_controller()->CreateDownloadTask(&web_state_, identifier, url,
                                            kContentDisposition, kMimeType);

  ASSERT_EQ(1U, observer_.alive_download_tasks().size());
  EXPECT_EQ(&web_state_, observer_.alive_download_tasks()[0].first);
  DownloadTask* task = observer_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);
  EXPECT_NSEQ(identifier, task->GetIndentifier());
  EXPECT_EQ(url, task->GetOriginalUrl());
  EXPECT_FALSE(task->IsDone());
  EXPECT_EQ(0, task->GetErrorCode());
  EXPECT_EQ(0, task->GetTotalBytes());
  EXPECT_EQ(0, task->GetPercentComplete());
  EXPECT_EQ(kContentDisposition, task->GetContentDisposition());
  EXPECT_EQ(kMimeType, task->GetMimeType());
  EXPECT_EQ("file.test", base::UTF16ToUTF8(task->GetSuggestedFilename()));
  base::Time remaining_time;
  EXPECT_FALSE(task->GetTimeRemaining(&remaining_time));
}

// Tests that destructing donwload task calls
// DownloadControllerObserver::OnDownloadUpdated.
TEST_F(DownloadControllerImplTest, OnDownloadUpdated) {
  NSString* identifier = [NSUUID UUID].UUIDString;
  download_controller()->CreateDownloadTask(&web_state_, identifier,
                                            GURL("https://download.test"),
                                            kContentDisposition, kMimeType);
  ASSERT_TRUE(observer_.destroyed_download_tasks().empty());
  ASSERT_EQ(1U, observer_.alive_download_tasks().size());

  DownloadTask* task = observer_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);

  NSURLSessionConfiguration* config = [NSURLSessionConfiguration
      backgroundSessionConfigurationWithIdentifier:identifier];
  NSURLSession* session = [NSURLSession sessionWithConfiguration:config];
  id<NSURLSessionDelegate> delegate = session.delegate;
  EXPECT_FALSE(delegate);
  // TODO: implement.
}

// Tests that destructing donwload task calls
// DownloadControllerObserver::OnDownloadDestroyed.
TEST_F(DownloadControllerImplTest, OnDownloadDestroyed) {
  download_controller()->CreateDownloadTask(
      &web_state_, [NSUUID UUID].UUIDString, GURL("https://download.test"),
      kContentDisposition, kMimeType);
  ASSERT_TRUE(observer_.destroyed_download_tasks().empty());
  ASSERT_EQ(1U, observer_.alive_download_tasks().size());

  DownloadTask* task = observer_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);

  observer_.RemoveDownloadTask(task);
  ASSERT_TRUE(observer_.alive_download_tasks().empty());
  ASSERT_EQ(1U, observer_.destroyed_download_tasks().size());
  EXPECT_EQ(task, observer_.destroyed_download_tasks()[0].second);
  EXPECT_EQ(&web_state_, observer_.destroyed_download_tasks()[0].first);
}

}  // namespace web
