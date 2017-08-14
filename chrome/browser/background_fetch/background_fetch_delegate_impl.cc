// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"

#include <utility>

#include "base/guid.h"
#include "base/strings/string_util.h"
#include "chrome/browser/download/download_service_factory.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/download_url_parameters.h"

namespace background_fetch {

BackgroundFetchDelegateImpl::BackgroundFetchDelegateImpl(
    content::BrowserContext* browser_context)
    : download_service_(
          DownloadServiceFactory::GetInstance()->GetForBrowserContext(
              browser_context)),
      weak_ptr_factory_(this) {}

BackgroundFetchDelegateImpl::~BackgroundFetchDelegateImpl() {}

void BackgroundFetchDelegateImpl::Shutdown() {}

void BackgroundFetchDelegateImpl::DownloadUrl(
    const std::string& guid,
    std::unique_ptr<content::DownloadUrlParameters> parameters,
    scoped_refptr<content::BackgroundFetchRequestInfo> request) {
  download::DownloadParams params;
  params.client = download::DownloadClient::BACKGROUND_FETCH;
  params.traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(NO_TRAFFIC_ANNOTATION_YET);
  params.request_params.url = parameters->url();
  params.guid = guid;
  params.callback = base::Bind(&BackgroundFetchDelegateImpl::OnStartDownload,
                               weak_ptr_factory_.GetWeakPtr());

  download_service_->StartDownload(params);
}

void BackgroundFetchDelegateImpl::OnDownloadStarted(
    const std::string& guid,
    std::unique_ptr<const content::BackgroundFetchResponse> response) {
  if (client_) {
    client_->OnDownloadStarted(guid, std::move(response));
  }
}

void BackgroundFetchDelegateImpl::OnDownloadUpdated(const std::string& guid,
                                                    uint64_t bytes_downloaded) {
  if (client_) {
    client_->OnDownloadUpdated(guid, bytes_downloaded);
  }
}

void BackgroundFetchDelegateImpl::OnDownloadFailed(const std::string& guid,
                                                   FailureReason reason) {
  if (client_) {
    client_->OnDownloadFailed(guid, reason);
  }
}

void BackgroundFetchDelegateImpl::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  if (client_) {
    client_->OnDownloadSucceeded(guid, path, size);
  }
}

void BackgroundFetchDelegateImpl::OnStartDownload(
    const std::string& guid,
    download::DownloadParams::StartResult result) {}

void BackgroundFetchDelegateImpl::SetDelegateClient(
    base::WeakPtr<content::BackgroundFetchDelegate::Client> client) {
  DCHECK(client.get() != nullptr);

  DCHECK(!client_);
  client_ = client;
}

}  // namespace background_fetch
