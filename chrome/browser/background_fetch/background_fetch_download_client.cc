// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_download_client.h"

#include <iostream>

#include "content/public/browser/background_fetch_delegate.h"
#include "content/public/browser/browser_context.h"

namespace background_fetch {

BackgroundFetchDownloadClient::BackgroundFetchDownloadClient(
    content::BrowserContext* context)
    : browser_context_(context) {}

BackgroundFetchDownloadClient::~BackgroundFetchDownloadClient() = default;

void BackgroundFetchDownloadClient::OnServiceInitialized(
    bool state_lost,
    const std::vector<std::string>& outstanding_download_guids) {
  delegate_ = browser_context_->GetBackgroundFetchDelegate();
}

void BackgroundFetchDownloadClient::OnServiceUnavailable() {}

download::Client::ShouldDownload
BackgroundFetchDownloadClient::OnDownloadStarted(
    const std::string& guid,
    const std::vector<GURL>& url_chain,
    const scoped_refptr<const net::HttpResponseHeaders>& headers) {
  delegate_->OnDownloadStarted(guid, url_chain, headers);
  return download::Client::ShouldDownload::CONTINUE;
}

void BackgroundFetchDownloadClient::OnDownloadUpdated(
    const std::string& guid,
    uint64_t bytes_downloaded) {
  std::cerr << "GUID: " << guid << " " << bytes_downloaded << "\n";
  delegate_->OnDownloadUpdated(guid, bytes_downloaded);
}

void BackgroundFetchDownloadClient::OnDownloadFailed(
    const std::string& guid,
    download::Client::FailureReason reason) {
  std::cerr << "GUID: " << guid << " failure: " << static_cast<int>(reason)
            << "\n";
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

    default:
      NOTREACHED();
  }
  delegate_->OnDownloadFailed(guid, failure_reason);
}

void BackgroundFetchDownloadClient::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  std::cerr << "GUID: " << guid << " succeeded " << path.LossyDisplayName()
            << " " << size << "\n";
  delegate_->OnDownloadSucceeded(guid, path, size);
}

}  // namespace background_fetch
