// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_cert_fetcher.h"

#include "base/base64.h"
#include "content/common/throttling_url_loader.h"
#include "net/base/hash_value.h"
#include "net/cert/cert_verifier.h"
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
    unverified_cert_ = certs[0];
    const net::SHA256HashValue fingerprint =
        net::X509Certificate::CalculateFingerprint256(
            unverified_cert_->cert_buffer());
    LOG(ERROR) << "HashValue " << net::HashValue(fingerprint).ToString();

    std::string base64_str;
    base::Base64Encode(
        base::StringPiece(reinterpret_cast<const char*>(fingerprint.data),
                          sizeof(fingerprint.data)),
        &base64_str);
    LOG(ERROR) << "base64_str " << base64_str;

    DCHECK(request_context_->cert_verifier());

    cert_verify_result_ = base::MakeUnique<net::CertVerifyResult>();
    cert_net_log_ = base::MakeUnique<net::NetLogWithSource>();

    const int return_value = request_context_->cert_verifier()->Verify(
        net::CertVerifier::RequestParams(unverified_cert_, "example.com",
                                         0 /* flag */, "", certs),
        net::SSLConfigService::GetCRLSet().get(), cert_verify_result_.get(),
        base::BindRepeating(&SignedExchangeCertFetcher::OnVerifyComplete,
                            base::Unretained(this)),
        &cert_verifier_request_, *cert_net_log_);
    LOG(ERROR) << "return_value " << return_value;
    if (return_value != net::ERR_IO_PENDING)
      OnVerifyComplete(return_value);
    return;
  }
  base::Optional<net::SSLInfo> ssl_info;
  std::move(callback_).Run(0, ssl_info);
}

void SignedExchangeCertFetcher::OnVerifyComplete(int result) {
  LOG(ERROR) << "SignedExchangeCertFetcher::OnVerifyComplete " << result;
  LOG(ERROR) << " cert_status: " << cert_verify_result_->cert_status;
  LOG(ERROR) << " public_key_hashes size: "
             << cert_verify_result_->public_key_hashes.size();
  LOG(ERROR) << " is_issued_by_known_root: "
             << cert_verify_result_->is_issued_by_known_root;
  LOG(ERROR) << " is_issued_by_additional_trust_anchor: "
             << cert_verify_result_->is_issued_by_additional_trust_anchor;
  LOG(ERROR) << " verified_cert: " << !!cert_verify_result_->verified_cert;
  net::SSLInfo ssl_info;
  ssl_info.cert = cert_verify_result_->verified_cert;
  ssl_info.unverified_cert = unverified_cert_;
  ssl_info.cert_status = cert_verify_result_->cert_status;
  ssl_info.is_issued_by_known_root =
      cert_verify_result_->is_issued_by_known_root;
  ssl_info.public_key_hashes = cert_verify_result_->public_key_hashes;
  ssl_info.ocsp_result = cert_verify_result_->ocsp_result;
  ssl_info.is_fatal_cert_error =
      net::IsCertStatusError(cert_verify_result_->cert_status) &&
      !net::IsCertStatusMinorError(cert_verify_result_->cert_status);

  std::move(callback_).Run(cert_verify_result_->cert_status, ssl_info);
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
  resource_request_->render_frame_id = -1;
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
