// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/preconnect_manager.h"

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;
using testing::StrictMock;

namespace predictors {

class MockPreconnectManagerDelegate
    : public PreconnectManager::Delegate,
      public base::SupportsWeakPtr<MockPreconnectManagerDelegate> {
 public:
  MOCK_METHOD1(PreconnectFinished, void(const GURL&));
};

class MockPreconnectManager : public PreconnectManager {
 public:
  MockPreconnectManager(
      base::WeakPtr<Delegate> delegate,
      scoped_refptr<net::URLRequestContextGetter> context_getter);

  MOCK_CONST_METHOD6(PreconnectUrl,
                     void(net::URLRequestContext* request_context,
                          const GURL& url,
                          const GURL& first_party_for_cookies,
                          int count,
                          bool allow_credentials,
                          net::HttpRequestInfo::RequestMotivation motivation));
  MOCK_CONST_METHOD3(PreresolveUrl,
                     int(net::URLRequestContext* request_context,
                         const GURL& url,
                         const net::CompletionCallback& callback));
};

MockPreconnectManager::MockPreconnectManager(
    base::WeakPtr<Delegate> delegate,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : PreconnectManager(delegate, context_getter) {}

class PreconnectManagerTest : public testing::Test {
 public:
  PreconnectManagerTest();
  ~PreconnectManagerTest() override;

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<StrictMock<MockPreconnectManagerDelegate>> mock_delegate_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  std::unique_ptr<StrictMock<MockPreconnectManager>> preconnect_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PreconnectManagerTest);
};

PreconnectManagerTest::PreconnectManagerTest()
    : mock_delegate_(
          base::MakeUnique<StrictMock<MockPreconnectManagerDelegate>>()),
      context_getter_(new net::TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get())),
      preconnect_manager_(base::MakeUnique<StrictMock<MockPreconnectManager>>(
          mock_delegate_->AsWeakPtr(),
          context_getter_)) {}

PreconnectManagerTest::~PreconnectManagerTest() = default;

TEST_F(PreconnectManagerTest, TestStartOneUrlPreresolve) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preresolve("http://cdn.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(_, url_to_preresolve, _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(*mock_delegate_, PreconnectFinished(main_frame_url));
  preconnect_manager_->Start(main_frame_url, {}, {url_to_preresolve});
  // Wait for PreconnectFinished task posted to the UI thread.
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStartOneUrlPreconnect) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preconnect("http://cdn.google.com");

  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(_, url_to_preconnect, _))
      .WillOnce(Return(net::OK));
  EXPECT_CALL(*preconnect_manager_,
              PreconnectUrl(_, url_to_preconnect, main_frame_url, 1, true,
                            net::HttpRequestInfo::PRECONNECT_MOTIVATED));
  EXPECT_CALL(*mock_delegate_, PreconnectFinished(main_frame_url));
  preconnect_manager_->Start(main_frame_url, {url_to_preconnect}, {});
  base::RunLoop().RunUntilIdle();
}

TEST_F(PreconnectManagerTest, TestStopOneUrlBeforePreconnect) {
  GURL main_frame_url("http://google.com");
  GURL url_to_preconnect("http://cdn.google.com");
  net::CompletionCallback callback;

  // Preconnect job isn't started before preresolve is completed asynchronously.
  EXPECT_CALL(*preconnect_manager_, PreresolveUrl(_, url_to_preconnect, _))
      .WillOnce(DoAll(SaveArg<2>(&callback), Return(net::ERR_IO_PENDING)));
  preconnect_manager_->Start(main_frame_url, {url_to_preconnect}, {});

  // Stop all jobs for |main_frame_url| before we get the callback.
  preconnect_manager_->Stop(main_frame_url);
  EXPECT_CALL(*mock_delegate_, PreconnectFinished(main_frame_url));
  callback.Run(net::OK);
  base::RunLoop().RunUntilIdle();
}

}  // namespace predictors
