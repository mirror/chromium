// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/test/mock_background_fetch_delegate.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

// -----------------------------------------------------------------------------
// TestResponse

TestResponse::TestResponse() = default;

TestResponse::~TestResponse() = default;

// -----------------------------------------------------------------------------
// TestResponseBuilder

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
    std::unique_ptr<DownloadUrlParameters> parameters,
    scoped_refptr<BackgroundFetchRequestInfo> request) {
  if (guid_responses_.find(guid) != guid_responses_.end()) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&Client::OnDownloadReceived, client_, guid,
                       BackgroundFetchDelegate::StartResult::UNEXPECTED_GUID));
    return;
  }

  // The URL wasn't registered, so fail with an internal error for lack of
  // anything better to do.
  if (url_responses_.find(parameters->url().spec()) == url_responses_.end()) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&Client::OnDownloadReceived, client_, guid,
                       BackgroundFetchDelegate::StartResult::INTERNAL_ERROR));
    return;
  }

  std::unique_ptr<TestResponse> test_response =
      std::move(url_responses_[parameters->url().spec()]);

  std::vector<GURL> urls({parameters->url()});
  scoped_refptr<const net::HttpResponseHeaders> headers;

  urls.push_back(parameters->url());
  headers = test_response->headers;

  std::unique_ptr<BackgroundFetchResponse> response =
      base::MakeUnique<BackgroundFetchResponse>(urls, headers);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&MockBackgroundFetchDelegate::OnDownloadStarted,
                     base::Unretained(this), guid, std::move(response)));

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
        base::BindOnce(&MockBackgroundFetchDelegate::OnDownloadSucceeded,
                       base::Unretained(this), guid, response_path,
                       test_response->data.size()));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&MockBackgroundFetchDelegate::OnDownloadFailed,
                       base::Unretained(this), guid, FailureReason::UNKNOWN));
  }

  guid_responses_[guid] = std::move(test_response);
}

void MockBackgroundFetchDelegate::OnDownloadStarted(
    const std::string& guid,
    std::unique_ptr<const BackgroundFetchResponse> response) {
  if (client_) {
    client_->OnDownloadStarted(guid, std::move(response));
  }
}

void MockBackgroundFetchDelegate::OnDownloadUpdated(const std::string& guid,
                                                    uint64_t bytes_downloaded) {
  NOTREACHED();
}

void MockBackgroundFetchDelegate::OnDownloadFailed(const std::string& guid,
                                                   FailureReason reason) {
  if (client_) {
    client_->OnDownloadFailed(guid, reason);
  }
}

void MockBackgroundFetchDelegate::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  std::unique_ptr<TestResponse> test_response =
      std::move(guid_responses_[guid]);

  ASSERT_NE(-1 /* error */, base::WriteFile(path, test_response->data.c_str(),
                                            test_response->data.size()));

  if (client_) {
    client_->OnDownloadSucceeded(guid, path, size);
  }
}

void MockBackgroundFetchDelegate::SetDelegateClient(
    base::WeakPtr<Client> client) {
  DCHECK(client.get() != nullptr);

  DCHECK(!client_) << "Delegate already set";
  client_ = client;
}

void MockBackgroundFetchDelegate::RegisterResponse(
    const std::string& url,
    std::unique_ptr<TestResponse> response) {
  url_responses_[url] = std::move(response);
}

}  // namespace content
