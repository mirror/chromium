// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "components/cronet/native/test_util.h"
#include "net/cert/mock_cert_verifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class EngineTest : public ::testing::Test {
 protected:
  EngineTest() = default;
  ~EngineTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(EngineTest);
};

TEST_F(EngineTest, StartCronetEngine) {
  Cronet_EnginePtr engine = Cronet_Engine_Create();
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EngineParams_set_userAgent(engine_params, "test");
  Cronet_Engine_StartWithParams(engine, engine_params);
  Cronet_Engine_Destroy(engine);
  Cronet_EngineParams_Destroy(engine_params);
}

TEST_F(EngineTest, CronetEngineDefaultUserAgent) {
  Cronet_EnginePtr engine = Cronet_Engine_Create();
  // Version and DefaultUserAgent don't require engine start.
  std::string version = Cronet_Engine_GetVersionString(engine);
  std::string default_agent = Cronet_Engine_GetDefaultUserAgent(engine);
  EXPECT_NE(default_agent.find(version), std::string::npos);
  Cronet_Engine_Destroy(engine);
}

TEST_F(EngineTest, InitDifferentEngines) {
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EnginePtr first_engine = Cronet_Engine_Create();
  Cronet_Engine_StartWithParams(first_engine, engine_params);
  Cronet_EnginePtr second_engine = Cronet_Engine_Create();
  Cronet_Engine_StartWithParams(second_engine, engine_params);
  Cronet_EnginePtr third_engine = Cronet_Engine_Create();
  Cronet_Engine_StartWithParams(third_engine, engine_params);
  Cronet_EngineParams_Destroy(engine_params);
  Cronet_Engine_Destroy(first_engine);
  Cronet_Engine_Destroy(second_engine);
  Cronet_Engine_Destroy(third_engine);
}

TEST_F(EngineTest, SetMockCertVerifierForTesting) {
  auto cert_verifier(std::make_unique<net::MockCertVerifier>());
  Cronet_EnginePtr engine = Cronet_Engine_Create();
  Cronet_Engine_SetMockCertVerifierForTesting(engine, cert_verifier.release());
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_Engine_StartWithParams(engine, engine_params);
  Cronet_Engine_Destroy(engine);
  Cronet_EngineParams_Destroy(engine_params);
}

}  // namespace
