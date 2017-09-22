// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_download_client.h"

#include <memory>
#include <utility>

#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"
#include "chrome/browser/download/download_service_factory.h"
#include "components/download/public/download_metadata.h"
#include "components/download/public/download_service.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_context.h"
#include "url/origin.h"

BackgroundFetchDownloadClient::BackgroundFetchDownloadClient(
    content::BrowserContext* context)
    : browser_context_(context) {}

BackgroundFetchDownloadClient::~BackgroundFetchDownloadClient() = default;

void BackgroundFetchDownloadClient::OnServiceInitialized(
    bool state_lost,
    const std::vector<download::DownloadMetaData>& downloads) {
  // TODO(delphick): Do something with outstanding downloads
  delegate_ = static_cast<BackgroundFetchDelegateImpl*>(
      browser_context_->GetBackgroundFetchDelegate());
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
  delegate_->OnDownloadFailed(guid, reason);
}

void BackgroundFetchDownloadClient::OnDownloadSucceeded(
    const std::string& guid,
    const download::CompletionInfo& info) {
  delegate_->OnDownloadSucceeded(guid, info.path, info.bytes_downloaded);
}

bool BackgroundFetchDownloadClient::CanServiceRemoveDownloadedFile(
    const std::string& guid,
    bool force_delete) {
  return true;
}
