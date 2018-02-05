// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cronet_c.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "components/cronet/native/test_url_request_callback.h"
#include "components/cronet/native/test_util.h"
#include "components/cronet/test/test_server.h"
#include "net/cert/mock_cert_verifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class UrlRequestTest : public ::testing::Test {
 protected:
  UrlRequestTest() {}
  ~UrlRequestTest() override {}

  void SetUp() override { cronet::TestServer::Start(); }

  void TearDown() override { cronet::TestServer::Shutdown(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestTest);
};

TEST_F(UrlRequestTest, SimpleRequest) {
  Cronet_EnginePtr engine = cronet::test::CreateTestEngine(0);
  Cronet_UrlRequestPtr request = Cronet_UrlRequest_Create();
  Cronet_UrlRequestParamsPtr request_params = Cronet_UrlRequestParams_Create();
  std::string url = cronet::TestServer::GetSimpleURL();

  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::test::CreateTestExecutor();
  cronet::test::TestUrlRequestCallback test_callback;
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback =
      test_callback.CreateUrlRequestCallback();

  Cronet_UrlRequest_InitWithParams(request, engine, url.c_str(), request_params,
                                   callback, executor);

  Cronet_UrlRequest_Start(request);

  test_callback.WaitForDone();
  ASSERT_TRUE(test_callback.IsDone());
  ASSERT_STREQ("The quick brown fox jumps over the lazy dog.",
               test_callback.response_as_string_.c_str());

  Cronet_UrlRequestParams_Destroy(request_params);
  Cronet_UrlRequest_Destroy(request);

  Cronet_Engine_Destroy(engine);
}

TEST_F(UrlRequestTest, CancelRequest) {
  Cronet_EnginePtr engine = cronet::test::CreateTestEngine(0);
  Cronet_UrlRequestPtr request = Cronet_UrlRequest_Create();
  Cronet_UrlRequestParamsPtr request_params = Cronet_UrlRequestParams_Create();
  std::string url = cronet::TestServer::GetSimpleURL();

  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::test::CreateTestExecutor();
  cronet::test::TestUrlRequestCallback test_callback;
  test_callback.set_failure(test_callback.CANCEL_SYNC,
                            test_callback.ON_RESPONSE_STARTED);
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback =
      test_callback.CreateUrlRequestCallback();

  Cronet_UrlRequest_InitWithParams(request, engine, url.c_str(), request_params,
                                   callback, executor);

  Cronet_UrlRequest_Start(request);

  test_callback.WaitForDone();
  ASSERT_TRUE(test_callback.IsDone());
  ASSERT_TRUE(test_callback.on_canceled_called_);
  ASSERT_FALSE(test_callback.on_error_called_);
  ASSERT_TRUE(test_callback.response_as_string_.empty());

  Cronet_UrlRequestParams_Destroy(request_params);
  Cronet_UrlRequest_Destroy(request);

  Cronet_Engine_Destroy(engine);
}

TEST_F(UrlRequestTest, FailedRequestHostNotFound) {
  Cronet_EnginePtr engine = cronet::test::CreateTestEngine(0);
  Cronet_UrlRequestPtr request = Cronet_UrlRequest_Create();
  Cronet_UrlRequestParamsPtr request_params = Cronet_UrlRequestParams_Create();
  std::string url = "https://notfound.example.com";

  // Executor provided by the application.
  Cronet_ExecutorPtr executor = cronet::test::CreateTestExecutor();
  cronet::test::TestUrlRequestCallback test_callback;
  // Callback provided by the application.
  Cronet_UrlRequestCallbackPtr callback =
      test_callback.CreateUrlRequestCallback();

  Cronet_UrlRequest_InitWithParams(request, engine, url.c_str(), request_params,
                                   callback, executor);

  Cronet_UrlRequest_Start(request);

  test_callback.WaitForDone();
  ASSERT_TRUE(test_callback.IsDone());
  ASSERT_TRUE(test_callback.on_error_called_);
  ASSERT_FALSE(test_callback.on_canceled_called_);

  ASSERT_TRUE(test_callback.response_as_string_.empty());
  ASSERT_EQ(nullptr, test_callback.last_response_info_);
  ASSERT_NE(nullptr, test_callback.last_error_);

  ASSERT_EQ(Cronet_Error_ERROR_CODE_ERROR_HOSTNAME_NOT_RESOLVED,
            Cronet_Error_get_errorCode(test_callback.last_error_));
  ASSERT_FALSE(
      Cronet_Error_get_immediatelyRetryable(test_callback.last_error_));
  ASSERT_STREQ("net::ERR_NAME_NOT_RESOLVED",
               Cronet_Error_get_message(test_callback.last_error_));
  ASSERT_EQ(-105,
            Cronet_Error_get_internalErrorCode(test_callback.last_error_));
  ASSERT_EQ(0,
            Cronet_Error_get_quicDetailedErrorCode(test_callback.last_error_));

  Cronet_UrlRequestParams_Destroy(request_params);
  Cronet_UrlRequest_Destroy(request);

  Cronet_Engine_Destroy(engine);
}

TEST_F(UrlRequestTest, PerfTest) {
  const int kTestIterations = 10;
  const int kDownloadSize = 19307439;  // used for internal server only

  Cronet_EnginePtr engine = Cronet_Engine_Create();
  Cronet_EngineParamsPtr engine_params = Cronet_EngineParams_Create();
  Cronet_Engine_StartWithParams(engine, engine_params);

  std::string url = cronet::TestServer::PrepareBigDataURL(kDownloadSize);

  base::Time start = base::Time::Now();

  for (int i = 0; i < kTestIterations; ++i) {
    // Executor provided by the application.
    Cronet_ExecutorPtr executor = cronet::test::CreateTestExecutor();

    Cronet_UrlRequestPtr request = Cronet_UrlRequest_Create();
    Cronet_UrlRequestParamsPtr request_params =
        Cronet_UrlRequestParams_Create();
    cronet::test::TestUrlRequestCallback test_callback;
    test_callback.set_accumulate_response_data(false);
    // Callback provided by the application.
    Cronet_UrlRequestCallbackPtr callback =
        test_callback.CreateUrlRequestCallback();

    Cronet_UrlRequest_InitWithParams(request, engine, url.c_str(),
                                     request_params, callback, executor);

    Cronet_UrlRequest_Start(request);
    test_callback.WaitForDone();

    ASSERT_TRUE(test_callback.IsDone());
    ASSERT_EQ(kDownloadSize, test_callback.response_data_length_);

    Cronet_UrlRequestParams_Destroy(request_params);
    Cronet_UrlRequest_Destroy(request);
  }
  base::Time end = base::Time::Now();
  base::TimeDelta delta = end - start;

  LOG(INFO) << "Total time " << delta.InMillisecondsF() << " ms";
  LOG(INFO) << "Single Iteration time "
            << delta.InMillisecondsF() / kTestIterations << " ms";

  double bytes_per_ms =
      kDownloadSize * kTestIterations / delta.InMillisecondsF();
  double megabytes_per_ms = bytes_per_ms / 1000000;
  double megabits_per_second = megabytes_per_ms * 8 * 1000;
  LOG(INFO) << "Average Throughput: " << megabits_per_second << " mbps";

  Cronet_EngineParams_Destroy(engine_params);
  Cronet_Engine_Destroy(engine);
  cronet::TestServer::ReleaseBigDataURL();
}

}  // namespace
