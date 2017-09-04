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
  FakeBackgroundFetchDelegate() : weak_ptr_factory_(this) {}

  void DownloadUrl(const std::string& guid,
                   const std::string& method,
                   const GURL& url,
                   const net::NetworkTrafficAnnotationTag& traffic_annotation,
                   const net::HttpRequestHeaders& headers) override {
    if (!client_)
      return;

    std::vector<GURL> urls = {GURL("http://www.example.com/index.html")};
    auto response = base::MakeUnique<BackgroundFetchResponse>(
        urls, base::MakeRefCounted<net::HttpResponseHeaders>("200 OK"));

    client_->OnDownloadStarted(guid, std::move(response));
    if (complete_downloads_) {
      auto result = base::MakeUnique<BackgroundFetchResult>(
          base::Time::Now(), base::FilePath(), 10);
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(&BackgroundFetchDelegate::Client::OnDownloadComplete,
                         client_, guid, std::move(result)));
    }
  }

  void SetDelegateClient(base::WeakPtr<Client> client) override {
    client_ = client;
  }

  void set_complete_downloads(bool complete_downloads) {
    complete_downloads_ = complete_downloads;
  }

  base::WeakPtr<Client> client() { return client_; }

 private:
  bool complete_downloads_ = true;
  base::WeakPtr<Client> client_;
  // FIXME!!!
 public:
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
      : delegate_proxy_(delegate_.weak_ptr_factory_.GetWeakPtr()) {}

 protected:
  FakeBackgroundFetchDelegate delegate_;
  BackgroundFetchDelegateProxy delegate_proxy_;
};

TEST_F(BackgroundFetchDelegateProxyTest, SetDelegate) {
  EXPECT_TRUE(delegate_.client().get());
}

TEST_F(BackgroundFetchDelegateProxyTest, StartRequest) {
  FakeController controller;
  ServiceWorkerFetchRequest fetch_request;
  auto request =
      base::MakeRefCounted<BackgroundFetchRequestInfo>(0, fetch_request);

  EXPECT_FALSE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);

  delegate_proxy_.StartRequest(controller.weak_ptr_factory_.GetWeakPtr(),
                               url::Origin(), request);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(controller.request_started_);
  EXPECT_TRUE(controller.request_completed_);
}

TEST_F(BackgroundFetchDelegateProxyTest, StartRequest_NotCompleted) {
  FakeController controller;
  ServiceWorkerFetchRequest fetch_request;
  auto request =
      base::MakeRefCounted<BackgroundFetchRequestInfo>(0, fetch_request);

  EXPECT_FALSE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);

  delegate_.set_complete_downloads(false);
  delegate_proxy_.StartRequest(controller.weak_ptr_factory_.GetWeakPtr(),
                               url::Origin(), request);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(controller.request_started_);
  EXPECT_FALSE(controller.request_completed_);
}

}  // namespace content
