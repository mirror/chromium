// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cronet/Cronet.h>
#include <Cronet/cronet_c.h>
#import <Foundation/Foundation.h>

#include "base/strings/stringprintf.h"
#include "components/cronet/ios/test/start_cronet.h"
#include "components/grpc_support/test/get_stream_engine.h"
#include "net/base/net_errors.h"
#include "net/cert/mock_cert_verifier.h"

@interface Cronet (ExposedForTesting)
+ (void)shutdownForTesting;
@end

namespace grpc_support {

Cronet_EnginePtr g_cronet_engine = nullptr;

stream_engine* GetTestStreamEngine(int port) {
  return Cronet_Engine_GetStreamEngine(g_cronet_engine);
  // return [Cronet getGlobalEngine];
}

void StartTestStreamEngine(int port) {
  cronet::StartCronet(port);

  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EngineParams_set_userAgent(engine_params, "test");
  // Add Host Resolver Rules.
  std::string host_resolver_rules = base::StringPrintf(
      "MAP test.example.com 127.0.0.1:%d,"
      "MAP notfound.example.com ~NOTFOUND",
      port);
  Cronet_EngineParams_set_experimentalOptions(
      engine_params,
      base::StringPrintf(
          "{ \"HostResolverRules\": { \"host_resolver_rules\" : \"%s\" } }",
          host_resolver_rules.c_str())
          .c_str());

  Cronet_EngineParams_set_enableQuic(engine_params, true);
  // Add QUIC Hint.
  Cronet_QuicHintPtr quic_hint = Cronet_QuicHint_Create();
  Cronet_QuicHint_set_host(quic_hint, "test.example.com");
  Cronet_QuicHint_set_port(quic_hint, 443);
  Cronet_QuicHint_set_alternatePort(quic_hint, 443);
  Cronet_EngineParams_add_quicHints(engine_params, quic_hint);
  // Create Cronet Engine.
  g_cronet_engine = Cronet_Engine_Create();
  // Set Mock Cert Verifier.
  auto cert_verifier(std::make_unique<net::MockCertVerifier>());
  cert_verifier->set_default_result(net::OK);
  Cronet_Engine_SetMockCertVerifierForTesting(g_cronet_engine,
                                              cert_verifier.release());
  // Start Cronet Engine.
  Cronet_Engine_StartWithParams(g_cronet_engine, engine_params);
  Cronet_EngineParams_Destroy(engine_params);
}

void ShutdownTestStreamEngine() {
  [Cronet shutdownForTesting];
  Cronet_Engine_Destroy(g_cronet_engine);
  g_cronet_engine = nullptr;
}

}  // namespace grpc_support
