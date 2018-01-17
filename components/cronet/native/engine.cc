// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/engine.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "components/cronet/cronet_global_state.h"
#include "components/cronet/native/generated/cronet.idl_impl_struct.h"
#include "components/cronet/native/include/cronet_c.h"
#include "components/cronet/url_request_context_config.h"
#include "components/cronet/version.h"
#include "net/base/hash_value.h"
#include "net/url_request/url_request_context_builder.h"

namespace cronet {

Cronet_EngineImpl::Cronet_EngineImpl()
    : context_initalized_(base::WaitableEvent::ResetPolicy::MANUAL,
                          base::WaitableEvent::InitialState::NOT_SIGNALED) {
  stream_engine_.obj = nullptr;
  stream_engine_.annotation = nullptr;
}

Cronet_EngineImpl::~Cronet_EngineImpl() {
  Shutdown();
}

void Cronet_EngineImpl::SetContext(Cronet_EngineContext context) {
  engine_context_ = context;
}

Cronet_EngineContext Cronet_EngineImpl::GetContext() {
  return engine_context_;
}

void Cronet_EngineImpl::StartWithParams(Cronet_EngineParamsPtr params) {
  cronet::EnsureInitialized();
  URLRequestContextConfigBuilder context_config_builder;
  context_config_builder.enable_quic = params->enableQuic;
  context_config_builder.enable_spdy = params->enableHttp2;
  context_config_builder.enable_brotli = params->enableBrotli;
  switch (params->httpCacheMode) {
    case Cronet_EngineParams_HTTP_CACHE_MODE_DISABLED:
      context_config_builder.http_cache = URLRequestContextConfig::DISABLED;
      break;
    case Cronet_EngineParams_HTTP_CACHE_MODE_IN_MEMORY:
      context_config_builder.http_cache = URLRequestContextConfig::MEMORY;
      break;
    case Cronet_EngineParams_HTTP_CACHE_MODE_DISK:
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

bool Cronet_EngineImpl::StartNetLogToFile(CharString fileName, bool logAll) {
  base::AutoLock lock(context_lock_);
  if (is_logging_ || !context_)
    return false;
  is_logging_ = context_->StartNetLogToFile(fileName, logAll);
  return is_logging_;
}

void Cronet_EngineImpl::StopNetLog() {
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

CharString Cronet_EngineImpl::GetVersionString() {
  return CRONET_VERSION;
}

CharString Cronet_EngineImpl::GetDefaultUserAgent() {
  // TODO(mef): Use platform-specific user-agent generator.
  return "Cronet/" CRONET_VERSION;
}

void Cronet_EngineImpl::Shutdown() {
  if (context_) {
    StopNetLog();
    context_.reset();
  }
}

Cronet_EngineImpl::Callback::Callback(Cronet_EngineImpl* engine)
    : engine_(engine) {
  DETACH_FROM_THREAD(network_thread_checker_);
}

Cronet_EngineImpl::Callback::~Callback() = default;

void Cronet_EngineImpl::Callback::OnInitNetworkThread() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  // It is possible that engine_->context_ is reset from main thread while
  // being intialized on network thread.
  base::AutoLock lock(engine_->context_lock_);
  if (engine_->context_) {
    // Initialize bidirectional stream engine for grpc on the network thread.
    context_getter_ = engine_->context_->GetURLRequestContextGetter();
    engine_->stream_engine_.obj = context_getter_.get();
    engine_->context_initalized_.Signal();
  }
}

void Cronet_EngineImpl::Callback::OnDestroyNetworkThread() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
}

void Cronet_EngineImpl::Callback::OnInitCertVerifierData(
    net::CertVerifier* cert_verifier,
    const std::string& cert_verifier_data) {}

void Cronet_EngineImpl::Callback::OnSaveCertVerifierData(
    net::CertVerifier* cert_verifier) {}

void Cronet_EngineImpl::Callback::OnEffectiveConnectionTypeChanged(
    net::EffectiveConnectionType effective_connection_type) {}

void Cronet_EngineImpl::Callback::OnRTTOrThroughputEstimatesComputed(
    int32_t http_rtt_ms,
    int32_t transport_rtt_ms,
    int32_t downstream_throughput_kbps) {}

void Cronet_EngineImpl::Callback::OnRTTObservation(
    int32_t rtt_ms,
    int32_t timestamp_ms,
    net::NetworkQualityObservationSource source) {}

void Cronet_EngineImpl::Callback::OnThroughputObservation(
    int32_t throughput_kbps,
    int32_t timestamp_ms,
    net::NetworkQualityObservationSource source) {}

void Cronet_EngineImpl::Callback::OnStopNetLogCompleted() {
  DCHECK_CALLED_ON_VALID_THREAD(network_thread_checker_);
  CHECK(engine_);
  DCHECK(engine_->is_logging_);
  base::AutoLock lock(engine_->context_lock_);
  engine_->is_logging_ = false;
  engine_->stop_netlog_completed_->Signal();
}

void Cronet_EngineImpl::SetMockCertVerifierForTesting(
    std::unique_ptr<net::CertVerifier> mock_cert_verifier) {
  CHECK(!context_);
  mock_cert_verifier_ = std::move(mock_cert_verifier);
}

stream_engine* Cronet_EngineImpl::GetBidirectionalStreamEngine() {
  context_initalized_.Wait();
  return &stream_engine_;
}

};  // namespace cronet

CRONET_EXPORT Cronet_EnginePtr Cronet_Engine_Create() {
  return new cronet::Cronet_EngineImpl();
}

CRONET_EXPORT void Cronet_Engine_SetMockCertVerifierForTesting(
    Cronet_EnginePtr engine,
    void* raw_mock_cert_verifier) {
  cronet::Cronet_EngineImpl* engine_impl =
      static_cast<cronet::Cronet_EngineImpl*>(engine);
  std::unique_ptr<net::CertVerifier> cert_verifier;
  cert_verifier.reset(static_cast<net::CertVerifier*>(raw_mock_cert_verifier));
  engine_impl->SetMockCertVerifierForTesting(std::move(cert_verifier));
}

CRONET_EXPORT stream_engine* Cronet_Engine_GetStreamEngine(
    Cronet_EnginePtr engine) {
  cronet::Cronet_EngineImpl* engine_impl =
      static_cast<cronet::Cronet_EngineImpl*>(engine);
  return engine_impl->GetBidirectionalStreamEngine();
}
