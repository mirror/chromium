// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_download_client.h"

#include <memory>
#include <utility>

#include "chrome/browser/download/download_service_factory.h"
#include "components/download/public/download_service.h"
#include "content/public/browser/background_fetch_delegate.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_context.h"
#include "url/origin.h"

namespace background_fetch {

BackgroundFetchDownloadClient::BackgroundFetchDownloadClient(
    content::BrowserContext* context)
    : browser_context_(context) {}

BackgroundFetchDownloadClient::~BackgroundFetchDownloadClient() = default;

void BackgroundFetchDownloadClient::OnServiceInitialized(
    bool state_lost,
    const std::vector<std::string>& outstanding_download_guids) {
  delegate_ = browser_context_->GetBackgroundFetchDelegate();
  download_service_ =
      DownloadServiceFactory::GetInstance()->GetForBrowserContext(
          browser_context_);
}

void BackgroundFetchDownloadClient::OnServiceUnavailable() {}

download::Client::ShouldDownload
BackgroundFetchDownloadClient::OnDownloadStarted(
    const std::string& guid,
    const std::vector<GURL>& url_chain,
    const scoped_refptr<const net::HttpResponseHeaders>& headers) {
  std::unique_ptr<content::BackgroundFetchResponse> response =
      base::MakeUnique<content::BackgroundFetchResponse>(url_chain, headers);

  delegate_->OnDownloadStarted(guid, std::move(response));

  return download::Client::ShouldDownload::CONTINUE;
}

void BackgroundFetchDownloadClient::OnDownloadUpdated(
    const std::string& guid,
    uint64_t bytes_downloaded) {
  delegate_->OnDownloadUpdated(guid, bytes_downloaded);
}

void BackgroundFetchDownloadClient::OnDownloadFailed(
    const std::string& guid,
    download::Client::FailureReason reason) {
  using FailureReason = content::BackgroundFetchDelegate::FailureReason;
  FailureReason failure_reason;

  switch (reason) {
    case download::Client::FailureReason::NETWORK:
      failure_reason = FailureReason::NETWORK;
      break;
    case download::Client::FailureReason::TIMEDOUT:
      failure_reason = FailureReason::TIMEDOUT;
      break;
    case download::Client::FailureReason::UNKNOWN:
      failure_reason = FailureReason::UNKNOWN;
      break;

    case download::Client::FailureReason::ABORTED:
    case download::Client::FailureReason::CANCELLED:
      // The client cancelled or aborted it so no need to notify it.
      return;
  }
  delegate_->OnDownloadFailed(guid, failure_reason);
}

void BackgroundFetchDownloadClient::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  delegate_->OnDownloadSucceeded(guid, path, size);
}

}  // namespace background_fetch
