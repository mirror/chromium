// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/test_util.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "net/base/net_errors.h"
#include "net/cert/mock_cert_verifier.h"

namespace {
// Implementation of PostTaskExecutor methods.
void TestExecutor_Execute(Cronet_ExecutorPtr self, Cronet_RunnablePtr command) {
  CHECK(self);
  DVLOG(1) << "Post Task";
  base::PostTask(FROM_HERE, base::BindOnce(
                                [](Cronet_RunnablePtr command) {
                                  Cronet_Runnable_Run(command);
                                  Cronet_Runnable_Destroy(command);
                                },
                                command));
}
}  // namespace

namespace cronet {
namespace test {

Cronet_EnginePtr CreateTestEngine(int quic_server_port) {
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EngineParams_set_userAgent(engine_params, "test");
  // Add Host Resolver Rules.
  std::string host_resolver_rules = base::StringPrintf(
      "MAP test.example.com 127.0.0.1:%d,"
      "MAP notfound.example.com ~NOTFOUND",
      quic_server_port);
  Cronet_EngineParams_set_experimentalOptions(
      engine_params,
      base::StringPrintf(
          "{ \"HostResolverRules\": { \"host_resolver_rules\" : \"%s\" } }",
          host_resolver_rules.c_str())
          .c_str());
  // Enable QUIC.
  Cronet_EngineParams_set_enableQuic(engine_params, true);
  // Add QUIC Hint.
  Cronet_QuicHintPtr quic_hint = Cronet_QuicHint_Create();
  Cronet_QuicHint_set_host(quic_hint, "test.example.com");
  Cronet_QuicHint_set_port(quic_hint, 443);
  Cronet_QuicHint_set_alternatePort(quic_hint, 443);
  Cronet_EngineParams_add_quicHints(engine_params, quic_hint);
  // Create Cronet Engine.
  Cronet_EnginePtr cronet_engine = Cronet_Engine_Create();
  // Set Mock Cert Verifier.
  auto cert_verifier(std::make_unique<net::MockCertVerifier>());
  cert_verifier->set_default_result(net::OK);
  Cronet_Engine_SetMockCertVerifierForTesting(cronet_engine,
                                              cert_verifier.release());
  // Start Cronet Engine.
  Cronet_Engine_StartWithParams(cronet_engine, engine_params);
  Cronet_EngineParams_Destroy(engine_params);
  return cronet_engine;
}

Cronet_ExecutorPtr CreateTestExecutor() {
  return Cronet_Executor_CreateStub(TestExecutor_Execute);
}

}  // namespace test
}  // namespace cronet
