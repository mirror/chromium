// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/base64.h"
#include "base/feature_list.h"
#include "content/browser/loader/resource_requester_info.h"
#include "content/browser/loader/signed_exchange_cert_fetcher.h"
#include "content/browser/loader/url_loader_factory_impl.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/common/weak_wrapper_shared_url_loader_factory.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/content_features.h"
#include "content/public/common/url_loader_throttle.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/hash_value.h"
#include "net/base/io_buffer.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "net/http/transport_security_state.h"
#include "net/url_request/url_request_context.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

namespace {

void GetContextsCallbackForCertificateFetcherForSignedExchange(
    ResourceContext* resource_context,
    net::URLRequestContext* request_context,
    ResourceType resource_type,
    ResourceContext** resource_context_out,
    net::URLRequestContext** request_context_out) {
  *resource_context_out = resource_context;
  *request_context_out = request_context;
}

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<net::SourceStream> upstream,
    ExchangeHeadersCallback headers_callback,
    URLLoaderFactoryGetter* default_url_loader_factory_getter,
    ResourceContext* resource_context,
    net::URLRequestContext* request_context,
    URLLoaderThrottlesGetter url_loader_throttles_getter)
    : net::FilterSourceStream(net::SourceStream::TYPE_NONE,
                              std::move(upstream)),
      headers_callback_(std::move(headers_callback)),
      default_url_loader_factory_getter_(default_url_loader_factory_getter),
      resource_context_(resource_context),
      request_context_(request_context),
      url_loader_throttles_getter_(std::move(url_loader_throttles_getter)),
      net_log_(net::NetLogWithSource::Make(
          request_context_->net_log(),
          net::NetLogSourceType::CERT_VERIFIER_JOB)),
      weak_factory_(this) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));

  // Triggering the first read (asynchronously) for header parsing.
  header_out_buf_ = base::MakeRefCounted<net::IOBufferWithSize>(1);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&SignedExchangeHandler::DoHeaderLoop,
                                weak_factory_.GetWeakPtr()));
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

int SignedExchangeHandler::FilterData(net::IOBuffer* output_buffer,
                                      int output_buffer_size,
                                      net::IOBuffer* input_buffer,
                                      int input_buffer_size,
                                      int* consumed_bytes,
                                      bool upstream_eof_reached) {
  *consumed_bytes = 0;

  original_body_string_.append(input_buffer->data(), input_buffer_size);
  *consumed_bytes += input_buffer_size;

  // We shouldn't write any data into the out buffer while we're
  // parsing the header.
  if (headers_callback_)
    return 0;

  if (upstream_eof_reached) {
    // Run the parser code if this part is run for the first time.
    // Now original_body_string_ has the entire body.
    // (Note that we may come here multiple times if output_buffer_size
    // is not big enough.
    // TODO(https://crbug.com/803774): Do the streaming instead.
    size_t size_to_copy =
        std::min(original_body_string_.size() - body_string_offset_,
                 base::checked_cast<size_t>(output_buffer_size));
    memcpy(output_buffer->data(),
           original_body_string_.data() + body_string_offset_, size_to_copy);
    body_string_offset_ += size_to_copy;
    return base::checked_cast<int>(size_to_copy);
  }

  return 0;
}

std::string SignedExchangeHandler::GetTypeAsString() const {
  return "HTXG";  // Tentative.
}

void SignedExchangeHandler::DoHeaderLoop() {
  // Run the internal read loop by ourselves until we finish
  // parsing the headers. (After that the caller should pumb
  // the Read() calls).
  DCHECK(headers_callback_);
  DCHECK(header_out_buf_);
  int rv = Read(header_out_buf_.get(), header_out_buf_->size(),
                base::BindRepeating(&SignedExchangeHandler::DidReadForHeaders,
                                    base::Unretained(this), false /* sync */));
  if (rv != net::ERR_IO_PENDING)
    DidReadForHeaders(true /* sync */, rv);
}

void SignedExchangeHandler::DidReadForHeaders(bool completed_syncly,
                                              int result) {
  if (MaybeRunHeadersCallback() || result < 0)
    return;
  DCHECK_EQ(0, result);
  if (completed_syncly) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SignedExchangeHandler::DoHeaderLoop,
                                  weak_factory_.GetWeakPtr()));
  } else {
    DoHeaderLoop();
  }
}

bool SignedExchangeHandler::MaybeRunHeadersCallback() {
  if (!headers_callback_)
    return false;

  // If this was the first read, fire the headers callback now.
  // TODO(https://crbug.com/803774): This is just for testing, we should
  // implement the CBOR parsing here.
  FillMockExchangeHeaders();

  // TODO(https://crbug.com/803774) Consume the bytes size that were
  // necessary to read out the headers.
  network::mojom::URLLoaderFactory* url_loader_factory = nullptr;
  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    DCHECK(default_url_loader_factory_getter_.get());
    url_loader_factory =
        default_url_loader_factory_getter_->GetNetworkFactory();
  } else {
    DCHECK(!default_url_loader_factory_getter_.get());
    url_loader_factory_impl_ = std::make_unique<URLLoaderFactoryImpl>(
        ResourceRequesterInfo::CreateForCertificateFetcherForSignedExchange(
            base::BindRepeating(
                &GetContextsCallbackForCertificateFetcherForSignedExchange,
                base::Unretained(resource_context_),
                base::Unretained(request_context_))));
    url_loader_factory = url_loader_factory_impl_.get();
  }
  DCHECK(url_loader_factory);
  DCHECK(url_loader_throttles_getter_);
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles =
      std::move(url_loader_throttles_getter_).Run();
  cert_fetcher_ = SignedExchangeCertFetcher::CreateAndStart(
      base::MakeRefCounted<WeakWrapperSharedURLLoaderFactory>(
          url_loader_factory),
      std::move(throttles),
      GURL("http://127.0.0.1:8000/loading/htxg/resources/example.com.cert_msg"),
      false,
      base::BindOnce(&SignedExchangeHandler::OnCertRecieved,
                     base::Unretained(this)));

  return true;
}

void SignedExchangeHandler::OnCertRecieved(
    scoped_refptr<net::X509Certificate> cert) {
  if (!cert) {
    LOG(ERROR) << "SignedExchangeHandler::OnCertRecieved null ";
    std::move(headers_callback_)
        .Run(request_url_, request_method_, response_head_,
             base::Optional<net::SSLInfo>());
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
      .Run(request_url_, request_method_, response_head_, ssl_info);
}

void SignedExchangeHandler::FillMockExchangeHeaders() {
  // TODO(https://crbug.com/803774): Get the request url by parsing CBOR format.
  request_url_ = GURL("https://example.com/test.html");
  // TODO(https://crbug.com/803774): Get the request method by parsing CBOR
  // format.
  request_method_ = "GET";
  // TODO(https://crbug.com/803774): Get more headers by parsing CBOR.
  scoped_refptr<net::HttpResponseHeaders> headers(
      new net::HttpResponseHeaders("HTTP/1.1 200 OK"));
  response_head_.headers = headers;
  response_head_.mime_type = "text/html";
}

}  // namespace content
