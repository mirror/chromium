// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_cert_fetcher.h"

#include "base/base64.h"
#include "base/strings/string_piece.h"
#include "content/common/throttling_url_loader.h"
#include "net/base/hash_value.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/http/transport_security_state.h"
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

}  // namespace

base::Optional<std::vector<base::StringPiece>>
SignedExchangeCertFetcher::GetCertChainFromMessage(
    const base::StringPiece& message) {
  base::StringPiece remaining_data = message;
  uint8_t cert_request_context_size = 0;
  if (!ConsumeByte(&remaining_data, &cert_request_context_size)) {
    DVLOG(1) << "Can't read certificate request request context size.";
    return base::nullopt;
  }
  if (cert_request_context_size != 0) {
    DVLOG(1) << "Invalid certificate request context size: "
             << static_cast<int>(cert_request_context_size);
    return base::nullopt;
  }
  uint32_t cert_list_size = 0;
  if (!Consume3Bytes(&remaining_data, &cert_list_size)) {
    DVLOG(1) << "Can't read certificate list size.";
    return base::nullopt;
  }

  if (cert_list_size != remaining_data.length()) {
    DVLOG(1) << "Certificate list size error: cert_list_size=" << cert_list_size
             << " remaining=" << remaining_data.length();
    return base::nullopt;
  }

  std::vector<base::StringPiece> certs;
  while (!remaining_data.empty()) {
    uint32_t cert_data_size = 0;
    if (!Consume3Bytes(&remaining_data, &cert_data_size)) {
      DVLOG(1) << "Can't read certificate data size.";
      return base::nullopt;
    }
    if (remaining_data.length() < cert_data_size) {
      DVLOG(1) << "Certificate data size error: cert_data_size="
               << cert_data_size << " remaining=" << remaining_data.length();
      return base::nullopt;
    }
    certs.emplace_back(remaining_data.substr(0, cert_data_size));
    remaining_data.remove_prefix(cert_data_size);

    uint16_t extensions_size = 0;
    if (!Consume2Bytes(&remaining_data, &extensions_size)) {
      DVLOG(1) << "Can't read extensions size.";
      return base::nullopt;
    }
    if (remaining_data.length() < extensions_size) {
      DVLOG(1) << "Extensions size error: extensions_size=" << extensions_size
               << " remaining=" << remaining_data.length();
      return base::nullopt;
    }
    remaining_data.remove_prefix(extensions_size);
  }
  return certs;
}

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
  base::Optional<std::vector<base::StringPiece>> der_certs =
      GetCertChainFromMessage(original_body_string_);
  if (der_certs)
    unverified_cert_ = net::X509Certificate::CreateFromDERCertChain(*der_certs);
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

  const int return_value = request_context_->cert_verifier()->Verify(
      net::CertVerifier::RequestParams(unverified_cert_, "example.com",
                                       0 /* flag */, "",
                                       net::CertificateList()),
      net::SSLConfigService::GetCRLSet().get(), cert_verify_result_.get(),
      base::BindRepeating(&SignedExchangeCertFetcher::OnVerifyComplete,
                          base::Unretained(this)),
      &cert_verifier_request_, net_log_);
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

  LOG(ERROR) << "cert_transparency_verifier()->Verify --- ";
  net::SignedCertificateTimestampAndStatusList scts;
  request_context_->cert_transparency_verifier()->Verify(
      "example.com", cert_verify_result_->verified_cert.get(),
      "" /* stapled_ocsp_response, */, "" /* sct_list_from_tls_extension */,
      &scts, net_log_);
  LOG(ERROR) << "cert_transparency_verifier()->Verify done --- ";
  LOG(ERROR) << " scts.size(): " << scts.size();

  net::SCTList verified_scts =
      net::ct::SCTsMatchingStatus(scts, net::ct::SCT_STATUS_OK);
  net::ct::CTPolicyCompliance policy_compliance =
      request_context_->ct_policy_enforcer()->CheckCompliance(
          cert_verify_result_->verified_cert.get(), verified_scts, net_log_);
  LOG(ERROR) << " policy_compliance: " << static_cast<int>(policy_compliance);

  net::TransportSecurityState::CTRequirementsStatus ct_requirement_status =
      request_context_->transport_security_state()->CheckCTRequirements(
          net::HostPortPair("example.com", 443),
          cert_verify_result_->is_issued_by_known_root,
          cert_verify_result_->public_key_hashes,
          cert_verify_result_->verified_cert.get(), unverified_cert_.get(),
          scts, net::TransportSecurityState::ENABLE_EXPECT_CT_REPORTS,
          policy_compliance);
  LOG(ERROR) << " ct_requirement_status: "
             << static_cast<int>(ct_requirement_status);

  // TODO: Update cert_verify_result_->cert_status by cheking policy_compliance
  // and ct_requirement_status. See: SSLClientSocketImpl::VerifyCT()
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
      callback_(std::move(callback)),
      net_log_(net::NetLogWithSource::Make(
          request_context_->net_log(),
          net::NetLogSourceType::CERT_VERIFIER_JOB)) {
  LOG(ERROR) << " force_fetch_: " << force_fetch_;
  // TODO: fill more data.
  resource_request_->url = cert_url;
  resource_request_->resource_type = RESOURCE_TYPE_CERT_FOR_SIGNED_EXCHANGE;
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
