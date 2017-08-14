// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/test/mock_background_fetch_delegate.h"
#include "net/http/http_response_headers.h"

namespace content {

MockBackgroundFetchDelegate::MockBackgroundFetchDelegate() {}

MockBackgroundFetchDelegate::~MockBackgroundFetchDelegate() {}

void MockBackgroundFetchDelegate::DownloadUrl(
    const std::string& guid,
    std::unique_ptr<DownloadUrlParameters> parameters,
    scoped_refptr<BackgroundFetchRequestInfo> request) {
  std::vector<GURL> urls;
  urls.push_back(parameters->url());
  scoped_refptr<const net::HttpResponseHeaders> headers =
      new net::HttpResponseHeaders("HTTP/1.1 200 OK\n");
  std::unique_ptr<BackgroundFetchResponse> response =
      base::MakeUnique<BackgroundFetchResponse>(urls, headers);
  client_->OnDownloadStarted(guid, std::move(response));

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&MockBackgroundFetchDelegate::OnDownloadSucceeded,
                 base::Unretained(this), guid, base::FilePath(), 64));
}

void MockBackgroundFetchDelegate::OnDownloadStarted(
    const std::string& guid,
    const std::unique_ptr<const BackgroundFetchResponse> response) {
  NOTREACHED();
}

void MockBackgroundFetchDelegate::OnDownloadUpdated(const std::string& guid,
                                                    uint64_t bytes_downloaded) {
  NOTREACHED();
}

void MockBackgroundFetchDelegate::OnDownloadFailed(const std::string& guid,
                                                   FailureReason reason) {
  NOTREACHED();
}

void MockBackgroundFetchDelegate::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  client_->OnDownloadSucceeded(guid, path, size);
}

void MockBackgroundFetchDelegate::SetDelegateClient(
    base::WeakPtr<Client> client) {
  DCHECK(client.get() != nullptr);

  DCHECK(!client_) << "Delegate already set";
  client_ = client;
}

}  // namespace content
