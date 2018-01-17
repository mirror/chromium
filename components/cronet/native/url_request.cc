// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/url_request.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "components/cronet/native/engine.h"
#include "components/cronet/native/generated/cronet.idl_impl_struct.h"
#include "components/cronet/native/include/cronet_c.h"
#include "components/cronet/native/runnables.h"

namespace {

net::RequestPriority ConvertRequestPriority(
    Cronet_UrlRequestParams_REQUEST_PRIORITY priority) {
  switch (priority) {
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_IDLE:
      return net::IDLE;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOWEST:
      return net::LOWEST;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_LOW:
      return net::LOW;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_MEDIUM:
      return net::MEDIUM;
    case Cronet_UrlRequestParams_REQUEST_PRIORITY_REQUEST_PRIORITY_HIGHEST:
      return net::HIGHEST;
  }
  return net::DEFAULT_PRIORITY;
}

std::unique_ptr<Cronet_UrlResponseInfo> ConvertCronet_UrlResponseInfo(
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server) {
  auto respone_info = std::make_unique<Cronet_UrlResponseInfo>();
  respone_info->url = "";
  respone_info->urlChain.push_back(respone_info->url);
  respone_info->httpStatusCode = http_status_code;
  // |headers| could be nullptr.
  if (headers != nullptr) {
    size_t iter = 0;
    std::string header_name;
    std::string header_value;
    while (headers->EnumerateHeaderLines(&iter, &header_name, &header_value)) {
      auto header = std::make_unique<Cronet_HttpHeader>();
      header->name = header_name;
      header->value = header_value;
      respone_info->allHeadersList.push_back(std::move(header));
    }
  }
  respone_info->wasCached = was_cached;
  respone_info->negotiatedProtocol = negotiated_protocol;
  respone_info->proxyServer = proxy_server;
  return respone_info;
}

}  // namespace

namespace cronet {

Cronet_UrlRequestImpl::Cronet_UrlRequestImpl() {}

Cronet_UrlRequestImpl::~Cronet_UrlRequestImpl() {
  request_->Destroy(false);
}

void Cronet_UrlRequestImpl::SetContext(Cronet_UrlRequestContext context) {
  url_request_context_ = context;
}

Cronet_UrlRequestContext Cronet_UrlRequestImpl::GetContext() {
  return url_request_context_;
}

void Cronet_UrlRequestImpl::InitWithParams(
    Cronet_EnginePtr engine,
    CharString url,
    Cronet_UrlRequestParamsPtr params,
    Cronet_UrlRequestCallbackPtr callback,
    Cronet_ExecutorPtr executor) {
  CHECK(engine);
  CHECK(callback);
  CHECK(executor);
  Cronet_EngineImpl* engine_impl = static_cast<Cronet_EngineImpl*>(engine);
  GURL gurl(url);
  VLOG(1) << "New Cronet_UrlRequest: " << gurl.possibly_invalid_spec();
  request_ = new CronetURLRequest(
      engine_impl->cronet_url_request_context(),
      std::make_unique<Callback>(this, callback, executor), gurl,
      ConvertRequestPriority(params->priority), params->disableCache,
      true /* params->disableConnectionMigration */,
      false /* params->enableMetrics */);

  if (!params->httpMethod.empty())
    request_->SetHttpMethod(params->httpMethod);

  for (const auto& request_header : params->requestHeaders) {
    request_->AddRequestHeader(request_header->name, request_header->value);
  }
}

void Cronet_UrlRequestImpl::Start() {
  request_->Start();
}
void Cronet_UrlRequestImpl::FollowRedirect() {
  request_->FollowDeferredRedirect();
}

void Cronet_UrlRequestImpl::Read(Cronet_BufferPtr buffer) {
  base::AutoLock lock(request_lock_);
  read_buffer_ = buffer;
  net::IOBuffer* io_buffer = new net::WrappedIOBuffer(
      reinterpret_cast<char*>(Cronet_Buffer_GetData(buffer)));
  request_->ReadData(io_buffer, Cronet_Buffer_GetSize(buffer));
}

void Cronet_UrlRequestImpl::Cancel() {
  request_->Destroy(true);
}

bool Cronet_UrlRequestImpl::IsDone() {
  return false;
}

void Cronet_UrlRequestImpl::GetStatus(
    Cronet_UrlRequestStatusListenerPtr listener) {}

Cronet_UrlRequestImpl::Callback::Callback(Cronet_UrlRequestImpl* url_request,
                                          Cronet_UrlRequestCallbackPtr callback,
                                          Cronet_ExecutorPtr executor)
    : url_request_(url_request), callback_(callback), executor_(executor) {
  DETACH_FROM_THREAD(network_thread_checker_);
}

Cronet_UrlRequestImpl::Callback::~Callback() {
  Cronet_Executor_Destroy(executor_);
  Cronet_UrlRequestCallback_Destroy(callback_);
}

// CronetURLRequest::Callback implementations:
void Cronet_UrlRequestImpl::Callback::OnReceivedRedirect(
    const std::string& new_location,
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

void Cronet_UrlRequestImpl::Callback::OnResponseStarted(
    int http_status_code,
    const std::string& http_status_text,
    const net::HttpResponseHeaders* headers,
    bool was_cached,
    const std::string& negotiated_protocol,
    const std::string& proxy_server) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  response_info_ = ConvertCronet_UrlResponseInfo(
      http_status_code, http_status_text, headers, was_cached,
      negotiated_protocol, proxy_server);
  // Invoke Cronet_UrlRequestCallback_OnResponseStarted using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnResponseStarted, callback_,
                     url_request_, response_info_.get()));
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnReadCompleted(
    scoped_refptr<net::IOBuffer> buffer,
    int bytes_read,
    int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  base::AutoLock lock(url_request_->request_lock_);
  response_info_->receivedByteCount = received_byte_count;
  Cronet_Buffer_SetPosition(url_request_->read_buffer_, bytes_read);
  // Invoke Cronet_UrlRequestCallback_OnReadCompleted using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(base::BindOnce(
      Cronet_UrlRequestCallback_OnReadCompleted, callback_, url_request_,
      response_info_.get(), url_request_->read_buffer_));
  url_request_->read_buffer_ = nullptr;
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnSucceeded(int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  response_info_->receivedByteCount = received_byte_count;
  // Invoke Cronet_UrlRequestCallback_OnSucceeded using OnceClosure
  Cronet_RunnablePtr runnable = new cronet::OnceClosureRunnable(
      base::BindOnce(Cronet_UrlRequestCallback_OnSucceeded, callback_,
                     url_request_, response_info_.get()));
  if (url_request_->read_buffer_) {
    Cronet_Buffer_Destroy(url_request_->read_buffer_);
    url_request_->read_buffer_ = nullptr;
  }
  // |runnable| is passed to executor, which destroys it after execution.
  Cronet_Executor_Execute(executor_, runnable);
}

void Cronet_UrlRequestImpl::Callback::OnError(int net_error,
                                              int quic_error,
                                              const std::string& error_string,
                                              int64_t received_byte_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  response_info_->receivedByteCount = received_byte_count;
}

void Cronet_UrlRequestImpl::Callback::OnCanceled() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

void Cronet_UrlRequestImpl::Callback::OnDestroyed() {
  DCHECK(url_request_);
}

void Cronet_UrlRequestImpl::Callback::OnMetricsCollected(
    const base::Time& request_start_time,
    const base::TimeTicks& request_start,
    const base::TimeTicks& dns_start,
    const base::TimeTicks& dns_end,
    const base::TimeTicks& connect_start,
    const base::TimeTicks& connect_end,
    const base::TimeTicks& ssl_start,
    const base::TimeTicks& ssl_end,
    const base::TimeTicks& send_start,
    const base::TimeTicks& send_end,
    const base::TimeTicks& push_start,
    const base::TimeTicks& push_end,
    const base::TimeTicks& receive_headers_end,
    const base::TimeTicks& request_end,
    bool socket_reused,
    int64_t sent_bytes_count,
    int64_t received_bytes_count) {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

};  // namespace cronet

CRONET_EXPORT Cronet_UrlRequestPtr Cronet_UrlRequest_Create() {
  return new cronet::Cronet_UrlRequestImpl();
}

/*
void Cronet_UrlRequestImpl::StartWithParams(Cronet_UrlRequestParamsPtr params) {
  cronet::EnsureInitialized();
  URLRequestContextConfigBuilder context_config_builder;
  context_config_builder.enable_quic = params->enableQuic;
  context_config_builder.enable_spdy = params->enableHttp2;
  context_config_builder.enable_brotli = params->enableBrotli;
  switch (params->httpCacheMode) {
    case Cronet_UrlRequestParams_HTTP_CACHE_MODE_DISABLED:
      context_config_builder.http_cache = URLRequestContextConfig::DISABLED;
      break;
    case Cronet_UrlRequestParams_HTTP_CACHE_MODE_IN_MEMORY:
      context_config_builder.http_cache = URLRequestContextConfig::MEMORY;
      break;
    case Cronet_UrlRequestParams_HTTP_CACHE_MODE_DISK:
      context_config_builder.http_cache = URLRequestContextConfig::DISK;
      break;
    default:
      context_config_builder.http_cache = URLRequestContextConfig::DISABLED;
  }
  context_config_builder.http_cache_max_size = params->httpCacheMaxSize;
  context_config_builder.storage_path = params->storagePath;
  context_config_builder.user_agent = params->userAgent;
  context_config_builder.experimental_options = params->experimentalOptions;
  context_config_builder.bypass_public_key_pinning_for_local_trust_anchors =
      params->enablePublicKeyPinningBypassForLocalTrustAnchors;
  // MockCertVerifier to use for testing purposes.
  context_config_builder.mock_cert_verifier = std::move(mock_cert_verifier_);
  std::unique_ptr<URLRequestContextConfig> config =
      context_config_builder.Build();

  for (const auto& publicKeyPins : params->publicKeyPins) {
    auto pkp = std::make_unique<URLRequestContextConfig::Pkp>(
        publicKeyPins->host, publicKeyPins->includeSubdomains,
        base::Time::FromJavaTime(publicKeyPins->expirationDate));
    for (const auto& pin_sha256 : publicKeyPins->pinsSha256) {
      net::HashValue pin_hash;
      if (pin_hash.FromString(pin_sha256))
        pkp->pin_hashes.push_back(pin_hash);
    }
    config->pkp_list.push_back(std::move(pkp));
  }

  for (const auto& quic_hint : params->quicHints) {
    config->quic_hints.push_back(
        std::make_unique<URLRequestContextConfig::QuicHint>(
            quic_hint->host, quic_hint->port, quic_hint->alternatePort));
  }

  net::URLRequestContextBuilder context_builder;
  context_builder.set_accept_language(params->acceptLanguage);

  context_ = std::make_unique<CronetURLRequestContext>(
      std::move(config), std::make_unique<Callback>(this));
  context_->InitRequestContextOnInitThread();
}

bool Cronet_UrlRequestImpl::StartNetLogToFile(CharString fileName,
                                              bool logAll) {
  base::AutoLock lock(context_lock_);
  if (is_logging_ || !context_)
    return false;
  is_logging_ = context_->StartNetLogToFile(fileName, logAll);
  return is_logging_;
}

void Cronet_UrlRequestImpl::StopNetLog() {
  {
    base::AutoLock lock(context_lock_);
    if (!is_logging_ || !context_)
      return;
    stop_netlog_completed_ = std::make_unique<base::WaitableEvent>(
        base::WaitableEvent::ResetPolicy::MANUAL,
        base::WaitableEvent::InitialState::NOT_SIGNALED);
    context_->StopNetLog();
    // Release |lock| so it could be acquired in OnStopNetLog.
  }
  stop_netlog_completed_->Wait();
  stop_netlog_completed_.reset();
}

CharString Cronet_UrlRequestImpl::GetVersionString() {
  return CRONET_VERSION;
}

CharString Cronet_UrlRequestImpl::GetDefaultUserAgent() {
  // TODO(mef): Use platform-specific user-agent generator.
  return "Cronet/" CRONET_VERSION;
}

void Cronet_UrlRequestImpl::Shutdown() {
  if (context_) {
    StopNetLog();
    context_.reset();
  }
}

Cronet_UrlRequestImpl::Callback::Callback(Cronet_UrlRequestImpl* UrlRequest)
    : UrlRequest_(UrlRequest) {
  DETACH_FROM_THREAD(network_thread_checker_);
}

Cronet_UrlRequestImpl::Callback::~Callback() = default;

void Cronet_UrlRequestImpl::Callback::OnInitNetworkThread() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  // It is possible that UrlRequest_->context_ is reset from main thread while
  // being intialized on network thread.
  base::AutoLock lock(UrlRequest_->context_lock_);
  if (UrlRequest_->context_) {
    // Initialize bidirectional stream UrlRequest for grpc on the network
    // thread.
    context_getter_ = UrlRequest_->context_->GetURLRequestContextGetter();
    UrlRequest_->stream_UrlRequest_.obj = context_getter_.get();
    UrlRequest_->context_initalized_.Signal();
  }
}

void Cronet_UrlRequestImpl::Callback::OnDestroyNetworkThread() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

void Cronet_UrlRequestImpl::Callback::OnInitCertVerifierData(
    net::CertVerifier* cert_verifier,
    const std::string& cert_verifier_data) {}

void Cronet_UrlRequestImpl::Callback::OnSaveCertVerifierData(
    net::CertVerifier* cert_verifier) {}

void Cronet_UrlRequestImpl::Callback::OnEffectiveConnectionTypeChanged(
    net::EffectiveConnectionType effective_connection_type) {}

void Cronet_UrlRequestImpl::Callback::OnRTTOrThroughputEstimatesComputed(
    int32_t http_rtt_ms,
    int32_t transport_rtt_ms,
    int32_t downstream_throughput_kbps) {}

void Cronet_UrlRequestImpl::Callback::OnRTTObservation(
    int32_t rtt_ms,
    int32_t timestamp_ms,
    net::NetworkQualityObservationSource source) {}

void Cronet_UrlRequestImpl::Callback::OnThroughputObservation(
    int32_t throughput_kbps,
    int32_t timestamp_ms,
    net::NetworkQualityObservationSource source) {}

void Cronet_UrlRequestImpl::Callback::OnStopNetLogCompleted() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  CHECK(UrlRequest_);
  DCHECK(UrlRequest_->is_logging_);
  base::AutoLock lock(UrlRequest_->context_lock_);
  UrlRequest_->is_logging_ = false;
  UrlRequest_->stop_netlog_completed_->Signal();
}

void Cronet_UrlRequestImpl::SetMockCertVerifierForTesting(
    std::unique_ptr<net::CertVerifier> mock_cert_verifier) {
  CHECK(!context_);
  mock_cert_verifier_ = std::move(mock_cert_verifier);
}

stream_UrlRequest* Cronet_UrlRequestImpl::GetBidirectionalStreamUrlRequest() {
  context_initalized_.Wait();
  return &stream_UrlRequest_;
}

};  // namespace cronet

CRONET_EXPORT Cronet_UrlRequestPtr Cronet_UrlRequest_Create() {
  return new cronet::Cronet_UrlRequestImpl();
}

CRONET_EXPORT void Cronet_UrlRequest_SetMockCertVerifierForTesting(
    Cronet_UrlRequestPtr UrlRequest,
    void* raw_mock_cert_verifier) {
  cronet::Cronet_UrlRequestImpl* UrlRequest_impl =
      static_cast<cronet::Cronet_UrlRequestImpl*>(UrlRequest);
  std::unique_ptr<net::CertVerifier> cert_verifier;
  cert_verifier.reset(static_cast<net::CertVerifier*>(raw_mock_cert_verifier));
  UrlRequest_impl->SetMockCertVerifierForTesting(std::move(cert_verifier));
}

CRONET_EXPORT stream_UrlRequest* Cronet_UrlRequest_GetStreamUrlRequest(
    Cronet_UrlRequestPtr UrlRequest) {
  cronet::Cronet_UrlRequestImpl* UrlRequest_impl =
      static_cast<cronet::Cronet_UrlRequestImpl*>(UrlRequest);
  return UrlRequest_impl->GetBidirectionalStreamUrlRequest();
}
*/
