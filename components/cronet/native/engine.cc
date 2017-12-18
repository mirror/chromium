// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/engine.h"

#include <utility>

#include "base/logging.h"
#include "base/macros.h"
#include "components/cronet/cronet_global_state.h"
#include "components/cronet/native/generated/cronet.idl_impl_struct.h"
#include "components/cronet/url_request_context_config.h"
#include "components/cronet/version.h"
#include "net/base/hash_value.h"
#include "net/url_request/url_request_context_builder.h"

namespace cronet {

Cronet_EngineImpl::Cronet_EngineImpl() = default;
Cronet_EngineImpl::~Cronet_EngineImpl() {
  Shutdown();
}

void Cronet_EngineImpl::SetContext(Cronet_EngineContext context) {
  context_ = context;
}

Cronet_EngineContext Cronet_EngineImpl::GetContext() {
  return context_;
}

void Cronet_EngineImpl::StartWithParams(Cronet_EngineParamsPtr params) {
  cronet::CronetGlobalState::EnsureInitialized();
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
  /*
  context_config_builder.mock_cert_verifier =
  std::move( mock_cert_verifier_);  // MockCertVerifier to use for testing
  purposes.
    */
  std::unique_ptr<URLRequestContextConfig> config =
      context_config_builder.Build();

  for (const auto& publicKeyPins : params->publicKeyPins) {
    // TODO(mef): Properly set expiration time.
    auto pkp = std::make_unique<URLRequestContextConfig::Pkp>(
        publicKeyPins->host, publicKeyPins->includeSubdomains, base::Time());
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

  main_context_ =
      std::make_unique<CronetURLRequestContext>(std::move(config), this);
  main_context_->InitRequestContextOnInitThread();
}

void Cronet_EngineImpl::StartNetLogToFile(CharString fileName, bool logAll) {
  CHECK(main_context_);
  main_context_->StartNetLogToFile(fileName, logAll);
}

void Cronet_EngineImpl::StopNetLog() {
  CHECK(main_context_);
  main_context_->StopNetLog();
}

CharString Cronet_EngineImpl::GetVersionString() {
  return CRONET_VERSION;
}

CharString Cronet_EngineImpl::GetDefaultUserAgent() {
  return "";
}

void Cronet_EngineImpl::Shutdown() {
  CHECK(main_context_);
  if (main_context_) {
    main_context_.release()->Destroy();
  }
}

void Cronet_EngineImpl::OnInitNetworkThread(CronetURLRequestContext* context) {}

void Cronet_EngineImpl::OnInitCertVerifierData(
    net::CertVerifier* cert_verifier,
    const std::string& cert_verifier_data) {}

void Cronet_EngineImpl::OnGetCertVerifierData(
    net::CertVerifier* cert_verifier) {}

void Cronet_EngineImpl::OnEffectiveConnectionTypeChanged(
    net::EffectiveConnectionType effective_connection_type) {}

void Cronet_EngineImpl::OnRTTOrThroughputEstimatesComputed(
    int32_t http_rtt_ms,
    int32_t transport_rtt_ms,
    int32_t downstream_throughput_kbps) {}

void Cronet_EngineImpl::OnRTTObservation(
    int32_t rtt_ms,
    int32_t timestamp_ms,
    net::NetworkQualityObservationSource source) {}

void Cronet_EngineImpl::OnThroughputObservation(
    int32_t throughput_kbps,
    int32_t timestamp_ms,
    net::NetworkQualityObservationSource source) {}

void Cronet_EngineImpl::OnStopNetLogCompleted() {}

};  // namespace cronet

CRONET_EXPORT Cronet_EnginePtr Cronet_Engine_Create() {
  return new cronet::Cronet_EngineImpl();
}
