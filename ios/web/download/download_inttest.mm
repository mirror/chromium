// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/download/download_controller.h"
#import "ios/web/public/download/download_task.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/test/fakes/fake_download_controller_observer.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/public/web_state/web_state.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_response.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

namespace {

const char kContentDisposition[] = "attachment; filename=download.test";
const char kMimeType[] = "application/vnd.test";

std::unique_ptr<net::test_server::HttpResponse> DownloadResponse(
    const net::test_server::HttpRequest& request) {
  auto result = base::MakeUnique<net::test_server::BasicHttpResponse>();
  result->set_code(net::HTTP_OK);
  result->AddCustomHeader("Content-Type", kMimeType);
  result->AddCustomHeader("Content-Disposition", kContentDisposition);
  return result;
}

}  // namespace

// Test fixture for DownloadController, DownloadControllerObserver and
// DownloadTask integration tests.
class DownloadTest : public WebTestWithWebState {
  void SetUp() override {
    WebTestWithWebState::SetUp();
    download_controller()->AddObserver(&observer_);
    server_.RegisterRequestHandler(base::Bind(&DownloadResponse));
    ASSERT_TRUE(server_.Start());
  }
  void TearDown() override {
    WebTestWithWebState::TearDown();
    download_controller()->RemoveObserver(&observer_);
  }

  DownloadController* download_controller() {
    return DownloadController::FromBrowserState(GetBrowserState());
  }

 protected:
  net::EmbeddedTestServer server_;
  FakeDownloadControllerObserver observer_;
};

// Tests sucessfull download flow.
TEST_F(DownloadTest, SucessfullDownload) {
  // Load download URL.
  GURL url(server_.GetURL("/"));
  web::NavigationManager::WebLoadParams params(url);
  web_state()->GetNavigationManager()->LoadURLWithParams(params);

  // Wait until download task is created.
  testing::WaitUntilConditionOrTimeout(testing::kWaitForPageLoadTimeout, ^{
    return !observer_.alive_download_tasks().empty();
  });
  ASSERT_EQ(1U, observer_.alive_download_tasks().size());

  // Verify the initial state of the download task.
  DownloadTask* task = observer_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);
  EXPECT_TRUE(task->GetIndentifier());
  EXPECT_EQ(url, task->GetOriginalUrl());
  EXPECT_FALSE(task->IsDone());
  EXPECT_EQ(0, task->GetErrorCode());
  EXPECT_EQ(0, task->GetTotalBytes());
  EXPECT_EQ(0, task->GetPercentComplete());
  EXPECT_EQ(kContentDisposition, task->GetContentDisposition());
  EXPECT_EQ(kMimeType, task->GetMimeType());
  EXPECT_EQ("download.test", base::UTF16ToUTF8(task->GetSuggestedFilename()));
  base::Time time;
  EXPECT_FALSE(task->GetTimeRemaining(&time));

  // Start the download task.
  task->Start();
}

}  // namespace web
