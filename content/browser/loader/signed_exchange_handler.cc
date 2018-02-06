// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/base64.h"
#include "base/feature_list.h"
#include "components/cbor/cbor_reader.h"
#include "content/browser/loader/merkle_integrity_source_stream.h"
#include "content/browser/loader/resource_requester_info.h"
#include "content/browser/loader/signed_exchange_cert_fetcher.h"
#include "content/browser/loader/url_loader_factory_impl.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/weak_wrapper_shared_url_loader_factory.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/hash_value.h"
#include "net/base/io_buffer.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/filter/source_stream.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/http/transport_security_state.h"
#include "net/url_request/url_request_context.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

namespace {

constexpr size_t kBufferSizeForRead = 65536;

// Field names defined in the application/http-exchange+cbor content type:
// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.5
constexpr char kHtxg[] = "htxg";
constexpr char kRequest[] = "request";
constexpr char kResponse[] = "response";
constexpr char kPayload[] = "payload";
constexpr char kUrlKey[] = ":url";
constexpr char kMethodKey[] = ":method";
constexpr char kStatusKey[] = ":status";

constexpr char kMiHeader[] = "MI";

cbor::CBORValue BytestringFromString(base::StringPiece in_string) {
  return cbor::CBORValue(
      std::vector<uint8_t>(in_string.begin(), in_string.end()));
}

bool IsStringEqualTo(const cbor::CBORValue& value, const char* str) {
  return value.is_string() && value.GetString() == str;
}

// TODO(https://crbug.com/803774): Just for now, remove once we have streaming
// CBOR parser.
class BufferSourceStream : public net::SourceStream {
 public:
  BufferSourceStream(const std::vector<uint8_t>& bytes)
      : net::SourceStream(SourceStream::TYPE_NONE), buf_(bytes), ptr_(0u) {}
  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    int bytes = std::min(static_cast<int>(buf_.size() - ptr_), buffer_size);
    memcpy(dest_buffer->data(), &buf_[ptr_], bytes);
    ptr_ += bytes;
    return bytes;
  }
  std::string Description() const override { return "buffer"; }

 private:
  std::vector<uint8_t> buf_;
  size_t ptr_;
};

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<net::SourceStream> body,
    ExchangeHeadersCallback headers_callback,
    network::mojom::URLLoaderFactoryPtrInfo url_loader_factory_for_browser,
    net::URLRequestContext* request_context)
    : headers_callback_(std::move(headers_callback)),
      source_(std::move(body)),
      request_context_(request_context),
      net_log_(net::NetLogWithSource::Make(
          request_context_->net_log(),
          net::NetLogSourceType::CERT_VERIFIER_JOB)),
      weak_factory_(this) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));
  url_loader_factory_for_browser_ptr_.Bind(
      std::move(url_loader_factory_for_browser));

  // Triggering the first read (asynchronously) for CBOR parsing.
  read_buf_ = base::MakeRefCounted<net::IOBufferWithSize>(kBufferSizeForRead);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&SignedExchangeHandler::ReadLoop,
                                weak_factory_.GetWeakPtr()));
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

void SignedExchangeHandler::ReadLoop() {
  DCHECK(headers_callback_);
  DCHECK(read_buf_);
  int rv = source_->Read(
      read_buf_.get(), read_buf_->size(),
      base::BindRepeating(&SignedExchangeHandler::DidRead,
                          base::Unretained(this), false /* sync */));
  if (rv != net::ERR_IO_PENDING)
    DidRead(true /* sync */, rv);
}

void SignedExchangeHandler::DidRead(bool completed_syncly, int result) {
  if (result < 0) {
    DVLOG(1) << "Error reading body stream: " << result;
    RunErrorCallback(static_cast<net::Error>(result));
    return;
  }

  if (result == 0) {
    if (!RunHeadersCallback())
      RunErrorCallback(net::ERR_FAILED);
    return;
  }

  original_body_string_.append(read_buf_->data(), result);

  if (completed_syncly) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SignedExchangeHandler::ReadLoop,
                                  weak_factory_.GetWeakPtr()));
  } else {
    ReadLoop();
  }
}

bool SignedExchangeHandler::RunHeadersCallback() {
  DCHECK(headers_callback_);

  cbor::CBORReader::DecoderError error;
  base::Optional<cbor::CBORValue> root = cbor::CBORReader::Read(
      base::span<const uint8_t>(
          reinterpret_cast<const uint8_t*>(original_body_string_.data()),
          original_body_string_.size()),
      &error);
  if (!root) {
    DVLOG(1) << "CBOR parsing failed: "
             << cbor::CBORReader::ErrorCodeToString(error);
    return false;
  }
  original_body_string_.clear();

  if (!root->is_array()) {
    DVLOG(1) << "CBOR root is not an array";
    return false;
  }
  const auto& root_array = root->GetArray();
  if (!IsStringEqualTo(root_array[0], kHtxg)) {
    DVLOG(1) << "CBOR has no htxg signature";
    return false;
  }

  if (!IsStringEqualTo(root_array[1], kRequest)) {
    DVLOG(1) << "request field not found";
    return false;
  }

  if (!root_array[2].is_map()) {
    DVLOG(1) << "request field is not a map";
    return false;
  }
  const auto& request_map = root_array[2].GetMap();

  // TODO(https://crbug.com/803774): request payload may come here.

  if (!IsStringEqualTo(root_array[3], kResponse)) {
    DVLOG(1) << "response field not found";
    return false;
  }

  if (!root_array[4].is_map()) {
    DVLOG(1) << "response field is not a map";
    return false;
  }
  const auto& response_map = root_array[4].GetMap();

  if (!IsStringEqualTo(root_array[5], kPayload)) {
    DVLOG(1) << "payload field not found";
    return false;
  }

  if (!root_array[6].is_bytestring()) {
    DVLOG(1) << "payload field is not a bytestring";
    return false;
  }
  const auto& payload_bytes = root_array[6].GetBytestring();

  auto url_iter = request_map.find(BytestringFromString(kUrlKey));
  if (url_iter == request_map.end() || !url_iter->second.is_bytestring()) {
    DVLOG(1) << ":url is not found or not a bytestring";
    return false;
  }
  request_url_ = GURL(url_iter->second.GetBytestringAsString());

  auto method_iter = request_map.find(BytestringFromString(kMethodKey));
  if (method_iter == request_map.end() ||
      !method_iter->second.is_bytestring()) {
    DVLOG(1) << ":method is not found or not a bytestring";
    return false;
  }
  request_method_ = std::string(method_iter->second.GetBytestringAsString());

  auto status_iter = response_map.find(BytestringFromString(kStatusKey));
  if (status_iter == response_map.end() ||
      !status_iter->second.is_bytestring()) {
    DVLOG(1) << ":status is not found or not a bytestring";
    return false;
  }
  base::StringPiece status_code_str =
      status_iter->second.GetBytestringAsString();

  std::string fake_header_str("HTTP/1.1 ");
  status_code_str.AppendToString(&fake_header_str);
  fake_header_str.append(" OK\r\n");
  for (const auto& it : response_map) {
    if (!it.first.is_bytestring() || !it.second.is_bytestring()) {
      DVLOG(1) << "Non-bytestring value in the response map";
      return false;
    }
    base::StringPiece name = it.first.GetBytestringAsString();
    base::StringPiece value = it.second.GetBytestringAsString();
    if (name == kMethodKey)
      continue;

    name.AppendToString(&fake_header_str);
    fake_header_str.append(": ");
    value.AppendToString(&fake_header_str);
    fake_header_str.append("\r\n");
  }
  fake_header_str.append("\r\n");
  response_head_.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(fake_header_str.c_str(),
                                        fake_header_str.size()));
  // TODO(https://crbug.com/803774): |mime_type| should be derived from
  // "Content-Type" header.
  response_head_.mime_type = "text/html";

  // TODO(https://crbug.com/803774): Check that the Signature header entry has
  // integrity="mi".
  std::string mi_header_value;
  if (!response_head_.headers->EnumerateHeader(nullptr, kMiHeader,
                                               &mi_header_value)) {
    DVLOG(1) << "Signed exchange has no MI: header";
    return false;
  }
  auto payload_stream = std::make_unique<BufferSourceStream>(payload_bytes);
  mi_stream_ = std::make_unique<MerkleIntegritySourceStream>(
      mi_header_value, std::move(payload_stream));

  DCHECK(url_loader_factory_for_browser_ptr_.get());
  cert_fetcher_ = SignedExchangeCertFetcher::CreateAndStart(
      base::MakeRefCounted<WeakWrapperSharedURLLoaderFactory>(
          url_loader_factory_for_browser_ptr_.get()),
      GURL("http://127.0.0.1:8000/loading/htxg/resources/example.com.cert_msg"),
      false,
      base::BindOnce(&SignedExchangeHandler::OnCertRecieved,
                     base::Unretained(this)));

  return true;
}

void SignedExchangeHandler::RunErrorCallback(net::Error error) {
  DCHECK(headers_callback_);
  std::move(headers_callback_)
      .Run(error, GURL(), std::string(), network::ResourceResponseHead(),
           nullptr, base::nullopt);
}

void SignedExchangeHandler::OnCertRecieved(
    scoped_refptr<net::X509Certificate> cert) {
  if (!cert) {
    LOG(ERROR) << "SignedExchangeHandler::OnCertRecieved null ";
    std::move(headers_callback_)
        .Run(net::OK, request_url_, request_method_, response_head_,
             std::move(mi_stream_), base::Optional<net::SSLInfo>());
    return;
  }
  unverified_cert_ = cert;
  {
    const net::SHA256HashValue fingerprint =
        net::X509Certificate::CalculateFingerprint256(cert->cert_buffer());
    std::string base64_str;
    base::Base64Encode(
        base::StringPiece(reinterpret_cast<const char*>(fingerprint.data),
                          sizeof(fingerprint.data)),
        &base64_str);
    LOG(ERROR) << "base64_str " << base64_str;
  }

  cert_verify_result_ = base::MakeUnique<net::CertVerifyResult>();

  const int return_value = request_context_->cert_verifier()->Verify(
      net::CertVerifier::RequestParams(unverified_cert_, "example.com",
                                       0 /* flag */, "",
                                       net::CertificateList()),
      net::SSLConfigService::GetCRLSet().get(), cert_verify_result_.get(),
      base::BindRepeating(&SignedExchangeHandler::OnCertVerifyComplete,
                          base::Unretained(this)),
      &cert_verifier_request_, net_log_);
  if (return_value != net::ERR_IO_PENDING)
    OnCertVerifyComplete(return_value);
}

void SignedExchangeHandler::OnCertVerifyComplete(int result) {
  LOG(ERROR) << "SignedExchangeHandler::OnVerifyComplete result: " << result;
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

  // TODO: Update ssl_info.cert by cheking policy_compliance and
  // ct_requirement_status. See: SSLClientSocketImpl::VerifyCT()

  std::move(headers_callback_)
      .Run(net::OK, request_url_, request_method_, response_head_,
           std::move(mi_stream_), ssl_info);
}

}  // namespace content
