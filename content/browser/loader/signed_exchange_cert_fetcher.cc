// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_cert_fetcher.h"

#include "base/base64.h"
#include "base/strings/string_piece.h"
#include "content/common/throttling_url_loader.h"
#include "net/base/hash_value.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/url_request/url_request_context.h"

namespace content {

namespace {

const net::NetworkTrafficAnnotationTag kCertFetcherTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("cert_fetcher", R"()");

// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html
// https://cs.chromium.org/chromium/src/third_party/boringssl/src/ssl/tls13_both.cc?type=cs&q=tls13_process_certificate&sq=package:chromium&l=105
// https://tools.ietf.org/html/draft-ietf-tls-tls13-23#section-4.4.2

bool ConsumeByte(base::StringPiece* data, uint8_t* out) {
  if (data->empty())
    return false;
  *out = (*data)[0];
  data->remove_prefix(1);
  return true;
}

bool Consume2Bytes(base::StringPiece* data, uint16_t* out) {
  if (data->size() < 2)
    return false;
  *out = (static_cast<uint8_t>((*data)[0]) << 8) |
         static_cast<uint8_t>((*data)[1]);
  data->remove_prefix(2);
  return true;
}

bool Consume3Bytes(base::StringPiece* data, uint32_t* out) {
  if (data->size() < 3)
    return false;
  *out = (static_cast<uint8_t>((*data)[0]) << 16) |
         (static_cast<uint8_t>((*data)[1]) << 8) |
         static_cast<uint8_t>((*data)[2]);
  data->remove_prefix(3);
  return true;
}

scoped_refptr<net::X509Certificate> CreateCertificateFromMessageString(
    const std::string& message) {
  base::StringPiece data = message;
  uint8_t certificate_request_context_size = 0;
  if (!ConsumeByte(&data, &certificate_request_context_size))
    return nullptr;
  // https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.6
  // Since this fetch is not in response to a CertificateRequest, the
  // certificate_request_context MUST be empty, and a non-empty value MUST cause
  // the parse to fail.
  if (certificate_request_context_size != 0)
    return nullptr;
  uint32_t certificate_list_size = 0;
  if (!Consume3Bytes(&data, &certificate_list_size))
    return nullptr;
  if (certificate_list_size != data.length())
    return nullptr;

  std::vector<base::StringPiece> der_certs;
  while (!data.empty()) {
    uint32_t cert_data_size = 0;
    if (!Consume3Bytes(&data, &cert_data_size))
      return nullptr;
    if (data.length() < cert_data_size)
      return nullptr;
    der_certs.emplace_back(data.substr(0, cert_data_size));
    data.remove_prefix(cert_data_size);

    uint16_t extensions_size = 0;
    if (!Consume2Bytes(&data, &extensions_size))
      return nullptr;
    if (data.length() < extensions_size)
      return nullptr;
    data.remove_prefix(extensions_size);
  }
  return net::X509Certificate::CreateFromDERCertChain(der_certs);
}

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
  unverified_cert_ = CreateCertificateFromMessageString(original_body_string_);

  if (!unverified_cert_) {
    std::move(callback_).Run(0, base::Optional<net::SSLInfo>());
    return;
  }

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
                                       0 /* flag */, "",
                                       net::CertificateList()),
      net::SSLConfigService::GetCRLSet().get(), cert_verify_result_.get(),
      base::BindRepeating(&SignedExchangeCertFetcher::OnVerifyComplete,
                          base::Unretained(this)),
      &cert_verifier_request_, *cert_net_log_);
  LOG(ERROR) << "return_value " << return_value;
  if (return_value != net::ERR_IO_PENDING)
    OnVerifyComplete(return_value);
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
