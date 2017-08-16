// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/guid.h"
#include "base/strings/string_util.h"
#include "chrome/browser/download/download_service_factory.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "content/public/browser/background_fetch_response.h"

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
    const std::string& method,
    const GURL& url,
    const net::HttpRequestHeaders& headers,
    scoped_refptr<content::BackgroundFetchRequestInfo> request) {
  download::DownloadParams params;
  params.guid = guid;
  params.client = download::DownloadClient::BACKGROUND_FETCH;
  params.request_params.method = method;
  params.request_params.url = url;
  params.request_params.request_headers = headers;
  params.callback = base::Bind(&BackgroundFetchDelegateImpl::OnDownloadReceived,
                               weak_ptr_factory_.GetWeakPtr());

  const net::NetworkTrafficAnnotationTag traffic_annotation(
      net::DefineNetworkTrafficAnnotation("background_fetch_context",
                                          R"(
          semantics {
            sender: "Background Fetch API"
            description:
              "The Background Fetch API enables developers to upload or "
              "download files on behalf of the user. Such fetches will yield "
              "a user visible notification to inform the user of the "
              "operation, through which it can be suspended, resumed and/or "
              "cancelled. The developer retains control of the file once the "
              "fetch is completed,  similar to XMLHttpRequest and other "
              "mechanisms for fetching resources using JavaScript."
            trigger:
              "When the website uses the Background Fetch API to request "
              "fetching a file and/or a list of files. This is a Web "
              "Platform API for which no express user permission is required."
            data:
              "The request headers and data as set by the website's "
              "developer."
            destination: WEBSITE
          }
          policy {
            cookies_allowed: YES
            cookies_store: "user"
            setting: "This feature cannot be disabled in settings."
            policy_exception_justification: "Not implemented."
          })"));

  params.traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(traffic_annotation);

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

void BackgroundFetchDelegateImpl::OnDownloadReceived(
    const std::string& guid,
    download::DownloadParams::StartResult result) {
  using StartResult = download::DownloadParams::StartResult;
  switch (result) {
    case StartResult::ACCEPTED:
      // Nothing to do.
      break;
    case StartResult::BACKOFF:
      // TODO(delphick): try again later?
      // TODO(delphick): Due to a bug at the moment, this happens all the time
      // because successful downloads are not removed, so don't NOTREACHED.
      break;
    case StartResult::UNEXPECTED_CLIENT:
      // This really should never happen since we're supplying the
      // DownloadClient.
      NOTREACHED();
    case StartResult::UNEXPECTED_GUID:
      // TODO(delphick): try again with a different GUID.
      NOTREACHED();
    case StartResult::CLIENT_CANCELLED:
      // TODO(delphick): do we need to do anything here, since we will have
      // cancelled it?
      break;
    case StartResult::INTERNAL_ERROR:
      // TODO(delphick): We need to handle this gracefully.
      NOTREACHED();
    case StartResult::COUNT:
      NOTREACHED();
  }
}

void BackgroundFetchDelegateImpl::SetDelegateClient(
    base::WeakPtr<content::BackgroundFetchDelegate::Client> client) {
  DCHECK(client.get() != nullptr);

  DCHECK(!client_);
  client_ = client;
}

}  // namespace background_fetch
