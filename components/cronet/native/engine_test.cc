// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
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

TEST_F(EngineTest, StartNetLogToFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath temp_path = base::MakeAbsoluteFilePath(temp_dir.GetPath());
  base::FilePath net_log_file =
      temp_path.Append(FILE_PATH_LITERAL("netlog.json"));

  Cronet_EnginePtr engine = Cronet_Engine_Create();
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_EngineParams_set_experimentalOptions(
      engine_params,
      "{ \"QUIC\" : {\"max_server_configs_stored_in_properties\" : 8} }");
  // Test that net log cannot start/stop before engine start.
  ASSERT_FALSE(Cronet_Engine_StartNetLogToFile(
      engine, net_log_file.value().c_str(), true));
  Cronet_Engine_StopNetLog(engine);

  // Start the engine.
  Cronet_Engine_StartWithParams(engine, engine_params);
  Cronet_EngineParams_Destroy(engine_params);

  // Test that normal start/stop net log works.
  ASSERT_TRUE(Cronet_Engine_StartNetLogToFile(
      engine, net_log_file.value().c_str(), true));
  Cronet_Engine_StopNetLog(engine);

  // Test that double start/stop net log works.
  ASSERT_TRUE(Cronet_Engine_StartNetLogToFile(
      engine, net_log_file.value().c_str(), true));
  // Test that second start fails.
  ASSERT_FALSE(Cronet_Engine_StartNetLogToFile(
      engine, net_log_file.value().c_str(), true));
  // Test that multiple stops work.
  Cronet_Engine_StopNetLog(engine);
  Cronet_Engine_StopNetLog(engine);
  Cronet_Engine_StopNetLog(engine);

  // Test that net log contains effective experimental options.
  std::string net_log;
  ASSERT_TRUE(base::ReadFileToString(net_log_file, &net_log));
  ASSERT_TRUE(
      net_log.find(
          "{\"QUIC\":{\"max_server_configs_stored_in_properties\":8}") !=
      std::string::npos);

  // Test that bad file name fails.
  ASSERT_FALSE(Cronet_Engine_StartNetLogToFile(engine, "bad/file/name", true));

  Cronet_Engine_Shutdown(engine);
  // Test that net log cannot start/stop after engine shutdown.
  ASSERT_FALSE(Cronet_Engine_StartNetLogToFile(
      engine, net_log_file.value().c_str(), true));
  Cronet_Engine_StopNetLog(engine);
  Cronet_Engine_Destroy(engine);
}

}  // namespace
