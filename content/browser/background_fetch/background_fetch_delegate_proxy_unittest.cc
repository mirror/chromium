// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/background_fetch/background_fetch_delegate_proxy.h"

#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/browser/background_fetch/background_fetch_delegate.h"
#include "content/browser/background_fetch/background_fetch_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class FakeBackgroundFetchDelegate : public BackgroundFetchDelegate {
 public:
  FakeBackgroundFetchDelegate(bool complete_downloads)
      : complete_downloads_(complete_downloads), weak_ptr_factory_(this) {}

  void DownloadUrl(const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override {
    if (client_) {
      std::vector<GURL> urls = {GURL("http://www.example.com/index.html")};
      auto headers = base::MakeRefCounted<net::HttpResponseHeaders>("200 OK");
      auto response = base::MakeUnique<BackgroundFetchResponse>(urls, headers);

      client_->OnDownloadStarted(guid, std::move(response));
      if (complete_downloads_) {
        auto result = base::MakeUnique<BackgroundFetchResult>(
            base::Time::Now(), base::FilePath("foo.txt"), 10);
        BrowserThread::PostTask(
            BrowserThread::IO, FROM_HERE,
            base::BindOnce(&BackgroundFetchDelegate::Client::OnDownloadComplete,
                           client_, guid, std::move(result)));
      }
    }
  }

  void SetDelegateClient(base::WeakPtr<Client> client) override {
    client_ = client;
  }

  bool complete_downloads_;
  base::WeakPtr<Client> client_;
  base::WeakPtrFactory<FakeBackgroundFetchDelegate> weak_ptr_factory_;
};

class FakeController : public BackgroundFetchDelegateProxy::Controller {
 public:
  FakeController() : weak_ptr_factory_(this) {}

  void DidStartRequest(scoped_refptr<BackgroundFetchRequestInfo> request,
                       const std::string& download_guid) override {
    request_started_ = true;
  }

  // Called when the given |request| has been completed.
  void DidCompleteRequest(
      scoped_refptr<BackgroundFetchRequestInfo> request) override {
    request_completed_ = true;
  }

  bool request_started_ = false;
  bool request_completed_ = false;
  base::WeakPtrFactory<FakeController> weak_ptr_factory_;
};

class BackgroundFetchDelegateProxyTest : public BackgroundFetchTestBase {
 public:
  BackgroundFetchDelegateProxyTest()
      : delegate_(base::MakeUnique<FakeBackgroundFetchDelegate>(false)) {}

  BackgroundFetchDelegateProxy delegate_proxy_;
  std::unique_ptr<FakeBackgroundFetchDelegate> delegate_;
};

TEST_F(BackgroundFetchDelegateProxyTest, SetDelegate) {
  EXPECT_FALSE(delegate_->client_.get());

  base::RunLoop run_loop;
  delegate_proxy_.SetDelegate(delegate_->weak_ptr_factory_.GetWeakPtr());
  run_loop.RunUntilIdle();

  EXPECT_TRUE(delegate_->client_.get());
}

TEST_F(BackgroundFetchDelegateProxyTest, StartRequest) {
  FakeController controller;
  ServiceWorkerFetchRequest fetch_request;
  auto request =
      base::MakeRefCounted<BackgroundFetchRequestInfo>(0, fetch_request);

  base::RunLoop run_loop;
  delegate_proxy_.SetDelegate(delegate_->weak_ptr_factory_.GetWeakPtr());
  delegate_proxy_.StartRequest(controller.weak_ptr_factory_.GetWeakPtr(),
                               url::Origin(), request);
  run_loop.RunUntilIdle();

  EXPECT_TRUE(controller.request_started_);
}

}  // namespace content
