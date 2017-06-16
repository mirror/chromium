// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/offline_prefetch_download_client.h"

#include "base/logging.h"
#include "chrome/browser/offline_pages/prefetch/prefetch_service_factory.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"

namespace offline_pages {

OfflinePrefetchDownloadClient::OfflinePrefetchDownloadClient(
    content::BrowserContext* context)
    : context_(context) {}

OfflinePrefetchDownloadClient::~OfflinePrefetchDownloadClient() {}

void OfflinePrefetchDownloadClient::OnServiceInitialized(
    const std::vector<std::string>& outstanding_download_guids) {}

download::Client::ShouldDownload
OfflinePrefetchDownloadClient::OnDownloadStarted(
    const std::string& guid,
    const std::vector<GURL>& url_chain,
    const scoped_refptr<const net::HttpResponseHeaders>& headers) {
  return download::Client::ShouldDownload::CONTINUE;
}

void OfflinePrefetchDownloadClient::OnDownloadUpdated(
    const std::string& guid,
    uint64_t bytes_downloaded) {}

void OfflinePrefetchDownloadClient::OnDownloadFailed(const std::string& guid) {
  GetPrefetchService()->OnDownloadCompleted(guid, false);
}

void OfflinePrefetchDownloadClient::OnDownloadTimedOut(
    const std::string& guid) {
  GetPrefetchService()->OnDownloadCompleted(guid, false);
}

void OfflinePrefetchDownloadClient::OnDownloadAborted(const std::string& guid) {
  GetPrefetchService()->OnDownloadCompleted(guid, false);
}

void OfflinePrefetchDownloadClient::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  GetPrefetchService()->OnDownloadCompleted(guid, true);
}

PrefetchService* OfflinePrefetchDownloadClient::GetPrefetchService() const {
  PrefetchService* prefetch_service =
      PrefetchServiceFactory::GetForBrowserContext(context_);
  DCHECK(prefetch_service);
  return prefetch_service;
}

}  // namespace offline_pages
