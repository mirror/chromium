// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_ENGINE_H_
#define COMPONENTS_CRONET_NATIVE_ENGINE_H_

#include <memory>

#include "base/macros.h"
#include "components/cronet/cronet_url_request_context.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace cronet {

// Implementation of Cronet_Engine that invokes callback on destroy.
class Cronet_EngineImpl : public Cronet_Engine,
                          public CronetURLRequestContext::Delegate {
 public:
  Cronet_EngineImpl();
  ~Cronet_EngineImpl() override;

  void SetContext(Cronet_EngineContext context) override;
  Cronet_EngineContext GetContext() override;

  void StartWithParams(Cronet_EngineParamsPtr params) override;
  void StartNetLogToFile(CharString fileName, bool logAll) override;
  void StopNetLog() override;
  CharString GetVersionString() override;
  CharString GetDefaultUserAgent() override;
  void Shutdown() override;

  // CronetURLRequestContext::Delegate
  void OnInitNetworkThread(CronetURLRequestContext* context) override;

  void OnInitCertVerifierData(net::CertVerifier* cert_verifier,
                              const std::string& cert_verifier_data) override;

  void OnGetCertVerifierData(net::CertVerifier* cert_verifier) override;

  void OnEffectiveConnectionTypeChanged(
      net::EffectiveConnectionType effective_connection_type) override;

  void OnRTTOrThroughputEstimatesComputed(
      int32_t http_rtt_ms,
      int32_t transport_rtt_ms,
      int32_t downstream_throughput_kbps) override;

  void OnRTTObservation(int32_t rtt_ms,
                        int32_t timestamp_ms,
                        net::NetworkQualityObservationSource source) override;

  void OnThroughputObservation(
      int32_t throughput_kbps,
      int32_t timestamp_ms,
      net::NetworkQualityObservationSource source) override;

  void OnStopNetLogCompleted() override;

 private:
  std::unique_ptr<CronetURLRequestContext> main_context_;
  scoped_refptr<net::URLRequestContextGetter> main_context_getter_;

  // Engine context.
  Cronet_EngineContext context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Cronet_EngineImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_ENGINE_H_
