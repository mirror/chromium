// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/background_fetch/mock_background_fetch_delegate.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_response_headers.h"

namespace content {

namespace {

void PostStartedResponse(
    const std::string& guid,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers) {
  std::unique_ptr<BackgroundFetchResponse> response =
      std::make_unique<BackgroundFetchResponse>(std::vector<GURL>({url}),
                                                response_headers);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&BackgroundFetchDelegate::Client::OnDownloadStarted,
                     client(), guid, std::move(response)));
}

void PostCompletedResponse(const std::string& guid,
                           const GURL& url,
                           std::unique_ptr<TestCompletedResponse> response) {
  if (response->succeeded) {
    base::FilePath response_path;
    if (!temp_directory_.IsValid())
      CHECK(temp_directory_.CreateUniqueTempDir());

    // Write the |response|'s data to a temporary file.
    CHECK(base::CreateTemporaryFileInDir(temp_directory_.GetPath(),
                                         &response_path));

    CHECK_NE(-1 /* error */,
             base::WriteFile(response_path, response->data.c_str(),
                             response->data.size()));

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BackgroundFetchDelegate::Client::OnDownloadComplete, client(),
            guid,
            std::make_unique<BackgroundFetchResult>(
                base::Time::Now(), response_path, response->data.size())));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BackgroundFetchDelegate::Client::OnDownloadComplete, client(),
            guid, std::make_unique<BackgroundFetchResult>(base::Time::Now())));
  }
}

}  // namespace

MockBackgroundFetchDelegate::TestCompletedResponse::TestCompletedResponse(
    bool succeeded,
    const std::string& data)
    : succeeded(succeeded), data(data) {}

MockBackgroundFetchDelegate::TestStartedResponseBuilder::
    TestStartedResponseBuilder(int response_code)
    : headers_(base::MakeRefCounted<net::HttpResponseHeaders>(
          "HTTP/1.1 " + base::IntToString(response_code))) {}

MockBackgroundFetchDelegate::TestStartedResponseBuilder&
MockBackgroundFetchDelegate::TestStartedResponseBuilder::AddResponseHeader(
    const std::string& name,
    const std::string& value) {
  DCHECK(headers_);
  headers_->AddHeader(name + ": " + value);
  return *this;
}

scoped_refptr<net::HttpResponseHeaders>
MockBackgroundFetchDelegate::TestStartedResponseBuilder::Build() {
  return std::move(headers_);
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
  DCHECK(seen_guids_.find(guid) == seen_guids_.end());
  seen_guids_.insert(guid);

  auto started_iter = started_responses_.find(url);
  if (started_iter != started_responses_.end()) {
    PostStartedResponse(guid, url, std::move(started_iter->second));
    started_responses_.erase(started_iter);
  } else {
    requests_awaiting_start_.emplace(url, guid);
  }

  auto completed_iter = completed_responses_.find(url);
  if (completed_iter != completed_responses_.end()) {
    PostCompletedResponse(guid, url, std::move(completed_iter->second));
    completed_responses_.erase(completed_iter);
  } else {
    requests_awaiting_complete_.emplace(url, guid);
  }
}

void MockBackgroundFetchDelegate::RegisterStartedResponse(
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers) {
  DCHECK_EQ(0u, started_responses_.count(url));
  auto iter = requests_awaiting_start_.find(url);
  if (iter != requests_awaiting_start_.end())
    PostStartedResponse(iter->second, url, std::move(response_headers));
  else
    started_responses_.emplace(url, std::move(response_headers));
}

void MockBackgroundFetchDelegate::RegisterCompletedResponse(
    const GURL& url,
    std::unique_ptr<TestCompletedResponse> response) {
  DCHECK_EQ(0u, completed_responses_.count(url));
  auto iter = requests_awaiting_complete_.find(url);
  if (iter != requests_awaiting_complete_.end())
    PostCompletedResponse(iter->second, url, std::move(response));
  else
    completed_responses_.emplace(url, std::move(response));
}

}  // namespace content
