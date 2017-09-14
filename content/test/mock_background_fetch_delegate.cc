// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_thread.h"
#include "content/test/mock_background_fetch_delegate.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TestResponse::TestResponse() = default;

TestResponse::~TestResponse() = default;

TestResponseBuilder::TestResponseBuilder(bool succeeded, int response_code)
    : response_(base::MakeUnique<TestResponse>()) {
  response_->succeeded_ = succeeded;
  response_->headers = make_scoped_refptr(new net::HttpResponseHeaders(
      "HTTP/1.1 " + std::to_string(response_code)));
}

TestResponseBuilder::~TestResponseBuilder() = default;

TestResponseBuilder& TestResponseBuilder::AddResponseHeader(
    const std::string& name,
    const std::string& value) {
  DCHECK(response_);
  response_->headers->AddHeader(name + ": " + value);
  return *this;
}

TestResponseBuilder& TestResponseBuilder::SetResponseData(std::string data) {
  DCHECK(response_);
  response_->data.swap(data);
  return *this;
}

std::unique_ptr<TestResponse> TestResponseBuilder::Build() {
  return std::move(response_);
}

MockBackgroundFetchDelegate::MockBackgroundFetchDelegate() {}

MockBackgroundFetchDelegate::~MockBackgroundFetchDelegate() {}

void MockBackgroundFetchDelegate::DownloadUrl(
    const std::string& guid,
    const std::string& method,
    const GURL& url,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    const net::HttpRequestHeaders& headers) {
  // TODO(delphick): Currently we just disallow re-using GUIDs but later when we
  // use the DownloadService, we should signal StartResult::UNEXPECTED_GUID.
  DCHECK(guid_responses_.find(guid) == guid_responses_.end());

  std::unique_ptr<TestResponse> test_response;
  scoped_refptr<const net::HttpResponseHeaders> response_headers;

  auto url_iter = url_responses_.find(url.spec());
  if (url_iter != url_responses_.end()) {
    test_response = std::move(url_iter->second);
    url_responses_.erase(url_iter);
  } else {
    // TODO(delphick): When we use the DownloadService, we should signal
    // StartResult::INTERNAL_ERROR to say the URL wasn't registered rather than
    // assuming 404.
    test_response = TestResponseBuilder(false, 404).Build();
  }

  response_headers = test_response->headers;

  std::unique_ptr<BackgroundFetchResponse> response =
      base::MakeUnique<BackgroundFetchResponse>(std::vector<GURL>({url}),
                                                response_headers);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&BackgroundFetchDelegate::Client::OnDownloadStarted,
                     client(), guid, std::move(response)));

  if (test_response->succeeded_) {
    base::FilePath response_path;
    if (!temp_directory_.IsValid()) {
      ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    }

    // Write the |response|'s data to a temporary file.
    ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_directory_.GetPath(),
                                               &response_path));

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BackgroundFetchDelegate::Client::OnDownloadComplete, client(),
            guid,
            std::make_unique<BackgroundFetchResult>(
                base::Time::Now(), response_path, test_response->data.size())));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BackgroundFetchDelegate::Client::OnDownloadComplete, client(),
            guid, std::make_unique<BackgroundFetchResult>(base::Time::Now())));
  }

  guid_responses_[guid] = std::move(test_response);
}

void MockBackgroundFetchDelegate::RegisterResponse(
    const std::string& url,
    std::unique_ptr<TestResponse> response) {
  url_responses_[url] = std::move(response);
}

}  // namespace content
