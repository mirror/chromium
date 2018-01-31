// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_cert_fetcher.h"

#include "content/common/throttling_url_loader.h"

namespace content {

namespace {

const net::NetworkTrafficAnnotationTag kCertFetcherTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("cert_fetcher", R"()");

}  // namespace

// static
std::unique_ptr<SignedExchangeCertFetcher>
SignedExchangeCertFetcher::CreateAndStart(
    scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    const GURL& cert_url,
    bool force_fetch,
    CertVerifiedCallback callback) {
  std::unique_ptr<SignedExchangeCertFetcher> cert_fetcher(
      new SignedExchangeCertFetcher(std::move(shared_url_loader_factory),
                                    std::move(throttles), cert_url, force_fetch,
                                    std::move(callback)));
  cert_fetcher->Start();
  return cert_fetcher;
}

SignedExchangeCertFetcher::~SignedExchangeCertFetcher() = default;

// network::mojom::URLLoaderClient
void SignedExchangeCertFetcher::OnReceiveResponse(
    const network::ResourceResponseHead& head,
    const base::Optional<net::SSLInfo>& ssl_info,
    network::mojom::DownloadedTempFilePtr downloaded_file) {
  LOG(ERROR) << "SignedExchangeCertFetcher::OnReceiveResponse";
  LOG(ERROR) << "content_length: " << head.content_length;
}
void SignedExchangeCertFetcher::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& head) {}
void SignedExchangeCertFetcher::OnDataDownloaded(int64_t data_length,
                                                 int64_t encoded_length) {}
void SignedExchangeCertFetcher::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback callback) {}
void SignedExchangeCertFetcher::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {}
void SignedExchangeCertFetcher::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {}
void SignedExchangeCertFetcher::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  LOG(ERROR) << "SignedExchangeCertFetcher::OnStartLoadingResponseBody";
}
void SignedExchangeCertFetcher::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  base::Optional<net::SSLInfo> ssl_info;
  std::move(callback_).Run(0, ssl_info);
}

SignedExchangeCertFetcher::SignedExchangeCertFetcher(
    scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    const GURL& cert_url,
    bool force_fetch,
    CertVerifiedCallback callback)
    : shared_url_loader_factory_(std::move(shared_url_loader_factory)),
      throttles_(std::move(throttles)),
      resource_request_(std::make_unique<network::ResourceRequest>()),
      force_fetch_(force_fetch),
      callback_(std::move(callback)) {
  LOG(ERROR) << " force_fetch_: " << force_fetch_;
  // TODO: fill more data.
  resource_request_->url = cert_url;
  resource_request_->resource_type = RESOURCE_TYPE_OBJECT;
  resource_request_->request_initiator = url::Origin::Create(cert_url);
}

void SignedExchangeCertFetcher::Start() {
  url_loader_ = ThrottlingURLLoader::CreateLoaderAndStart(
      std::move(shared_url_loader_factory_), std::move(throttles_),
      0 /* routing_id */, 0 /* request_id? */,
      network::mojom::kURLLoadOptionNone, resource_request_.get(), this,
      kCertFetcherTrafficAnnotation, base::ThreadTaskRunnerHandle::Get());
  // TODO
}

}  // namespace content
