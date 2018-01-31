// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_cert_fetcher.h"

#include "base/base64.h"
#include "content/common/throttling_url_loader.h"
#include "net/base/hash_value.h"
#include "net/cert/x509_certificate.h"
#include "net/url_request/url_request_context.h"

namespace content {

namespace {

const net::NetworkTrafficAnnotationTag kCertFetcherTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("cert_fetcher", R"()");

}  // namespace

// static
std::unique_ptr<SignedExchangeCertFetcher>
SignedExchangeCertFetcher::CreateAndStart(
    scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
    net::URLRequestContext* request_context,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    const GURL& cert_url,
    bool force_fetch,
    CertVerifiedCallback callback) {
  std::unique_ptr<SignedExchangeCertFetcher> cert_fetcher(
      new SignedExchangeCertFetcher(
          std::move(shared_url_loader_factory), request_context,
          std::move(throttles), cert_url, force_fetch, std::move(callback)));
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
  drainer_.reset(new mojo::common::DataPipeDrainer(this, std::move(body)));
}

void SignedExchangeCertFetcher::OnComplete(
    const network::URLLoaderCompletionStatus& status) {}

void SignedExchangeCertFetcher::OnDataAvailable(const void* data,
                                                size_t num_bytes) {
  LOG(ERROR) << "SignedExchangeCertFetcher::OnDataAvailable " << num_bytes;
  if (original_body_string_.size() + num_bytes >= 1024 * 1024) {
    // TODO: Add test for huge file.
    drainer_ = nullptr;
    std::move(callback_).Run(net::CERT_STATUS_INVALID,
                             base::Optional<net::SSLInfo>());
    return;
  }
  original_body_string_.append(static_cast<const char*>(data), num_bytes);
}

void SignedExchangeCertFetcher::OnDataComplete() {
  LOG(ERROR) << "SignedExchangeCertFetcher::OnDataComplete";
  if (data_completed_)
    return;
  data_completed_ = true;
  // LOG(ERROR) << "original_body_string_ " << original_body_string_;
  net::CertificateList certs =
      net::X509Certificate::CreateCertificateListFromBytes(
          original_body_string_.data(), original_body_string_.size(),
          net::X509Certificate::FORMAT_AUTO /* TODO */);
  LOG(ERROR) << "  certs  " << certs.size();
  if (!certs.empty()) {
    const net::SHA256HashValue fingerprint =
        net::X509Certificate::CalculateFingerprint256(certs[0]->cert_buffer());
    LOG(ERROR) << "HashValue " << net::HashValue(fingerprint).ToString();

    std::string base64_str;
    base::Base64Encode(
        base::StringPiece(reinterpret_cast<const char*>(fingerprint.data),
                          sizeof(fingerprint.data)),
        &base64_str);
    LOG(ERROR) << "base64_str " << base64_str;

    DCHECK(request_context_->cert_verifier());
  }
  base::Optional<net::SSLInfo> ssl_info;
  std::move(callback_).Run(0, ssl_info);
}

SignedExchangeCertFetcher::SignedExchangeCertFetcher(
    scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory,
    net::URLRequestContext* request_context,
    std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
    const GURL& cert_url,
    bool force_fetch,
    CertVerifiedCallback callback)
    : shared_url_loader_factory_(std::move(shared_url_loader_factory)),
      request_context_(request_context),
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
