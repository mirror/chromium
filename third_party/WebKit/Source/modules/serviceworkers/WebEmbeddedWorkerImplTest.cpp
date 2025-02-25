// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebEmbeddedWorker.h"

#include <memory>
#include "platform/WaitableEvent.h"
#include "platform/loader/fetch/ResourceError.h"
#include "platform/runtime_enabled_features.h"
#include "platform/testing/URLTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/Platform.h"
#include "public/platform/WebContentSettingsClient.h"
#include "public/platform/WebURLLoaderMockFactory.h"
#include "public/platform/WebURLResponse.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerInstalledScriptsManager.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerNetworkProvider.h"
#include "public/platform/modules/serviceworker/WebServiceWorkerProvider.h"
#include "public/web/WebEmbeddedWorkerStartData.h"
#include "public/web/WebSettings.h"
#include "public/web/modules/serviceworker/WebServiceWorkerContextClient.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/common/message_port/message_port_channel.h"

namespace blink {
namespace {

class MockServiceWorkerContextClient : public WebServiceWorkerContextClient {
 public:
  MockServiceWorkerContextClient() = default;
  ~MockServiceWorkerContextClient() override = default;

  MOCK_METHOD0(WorkerReadyForInspection, void());
  MOCK_METHOD0(WorkerContextFailedToStart, void());
  MOCK_METHOD0(WorkerScriptLoaded, void());

  void DidEvaluateWorkerScript(bool /* success */) override {
    script_evaluated_event_.Signal();
  }

  // Work-around for mocking a method that return unique_ptr.
  MOCK_METHOD0(CreateServiceWorkerNetworkProviderProxy,
               WebServiceWorkerNetworkProvider*());
  MOCK_METHOD0(CreateServiceWorkerProviderProxy, WebServiceWorkerProvider*());
  std::unique_ptr<WebServiceWorkerNetworkProvider>
  CreateServiceWorkerNetworkProvider() override {
    return std::unique_ptr<WebServiceWorkerNetworkProvider>(
        CreateServiceWorkerNetworkProviderProxy());
  }
  std::unique_ptr<WebServiceWorkerProvider> CreateServiceWorkerProvider()
      override {
    return std::unique_ptr<WebServiceWorkerProvider>(
        CreateServiceWorkerProviderProxy());
  }
  void GetClient(const WebString&,
                 std::unique_ptr<WebServiceWorkerClientCallbacks>) override {
    NOTREACHED();
  }
  void GetClients(const WebServiceWorkerClientQueryOptions&,
                  std::unique_ptr<WebServiceWorkerClientsCallbacks>) override {
    NOTREACHED();
  }
  void OpenNewTab(const WebURL&,
                  std::unique_ptr<WebServiceWorkerClientCallbacks>) override {
    NOTREACHED();
  }
  void OpenPaymentHandlerWindow(
      const WebURL&,
      std::unique_ptr<WebServiceWorkerClientCallbacks>) override {
    NOTREACHED();
  }
  void PostMessageToClient(const WebString& uuid,
                           TransferableMessage) override {
    NOTREACHED();
  }
  void SkipWaiting(
      std::unique_ptr<WebServiceWorkerSkipWaitingCallbacks>) override {
    NOTREACHED();
  }
  void Claim(std::unique_ptr<WebServiceWorkerClientsClaimCallbacks>) override {
    NOTREACHED();
  }
  void Focus(const WebString& uuid,
             std::unique_ptr<WebServiceWorkerClientCallbacks>) override {
    NOTREACHED();
  }
  void Navigate(const WebString& uuid,
                const WebURL&,
                std::unique_ptr<WebServiceWorkerClientCallbacks>) override {
    NOTREACHED();
  }
  void WorkerContextDestroyed() override { termination_event_.Signal(); }

  void WaitUntilScriptEvaluated() { script_evaluated_event_.Wait(); }
  void WaitUntilThreadTermination() { termination_event_.Wait(); }

 private:
  WaitableEvent script_evaluated_event_;
  WaitableEvent termination_event_;
};

class MockServiceWorkerInstalledScriptsManager
    : public WebServiceWorkerInstalledScriptsManager {
 public:
  MOCK_CONST_METHOD1(IsScriptInstalled, bool(const WebURL& script_url));
  MOCK_METHOD1(GetRawScriptData,
               std::unique_ptr<RawScriptData>(const WebURL& script_url));
};

class WebEmbeddedWorkerImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto client = std::make_unique<MockServiceWorkerContextClient>();
    auto installed_scripts_manager =
        std::make_unique<MockServiceWorkerInstalledScriptsManager>();
    mock_client_ = client.get();
    if (RuntimeEnabledFeatures::ServiceWorkerScriptStreamingEnabled()) {
      mock_installed_scripts_manager_ = installed_scripts_manager.get();
    } else {
      mock_installed_scripts_manager_ = nullptr;
    }
    worker_ = WebEmbeddedWorker::Create(
        std::move(client), std::move(installed_scripts_manager),
        mojo::ScopedMessagePipeHandle(), mojo::ScopedMessagePipeHandle());

    WebURL script_url = URLTestHelpers::ToKURL("https://www.example.com/sw.js");
    WebURLResponse response;
    response.SetMIMEType("text/javascript");
    response.SetHTTPStatusCode(200);
    Platform::Current()->GetURLLoaderMockFactory()->RegisterURL(script_url,
                                                                response, "");

    start_data_.script_url = script_url;
    start_data_.user_agent = WebString("dummy user agent");
    start_data_.pause_after_download_mode =
        WebEmbeddedWorkerStartData::kDontPauseAfterDownload;
    start_data_.wait_for_debugger_mode =
        WebEmbeddedWorkerStartData::kDontWaitForDebugger;
    start_data_.v8_cache_options = WebSettings::kV8CacheOptionsDefault;
  }

  void TearDown() override {
    Platform::Current()
        ->GetURLLoaderMockFactory()
        ->UnregisterAllURLsAndClearMemoryCache();
  }

  WebEmbeddedWorkerStartData start_data_;
  MockServiceWorkerContextClient* mock_client_;
  MockServiceWorkerInstalledScriptsManager* mock_installed_scripts_manager_;
  std::unique_ptr<WebEmbeddedWorker> worker_;
};

}  // namespace

TEST_F(WebEmbeddedWorkerImplTest, TerminateSoonAfterStart) {
  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(1);
  worker_->TerminateWorkerContext();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
}

TEST_F(WebEmbeddedWorkerImplTest, TerminateWhileWaitingForDebugger) {
  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  start_data_.wait_for_debugger_mode =
      WebEmbeddedWorkerStartData::kWaitForDebugger;
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(1);
  worker_->TerminateWorkerContext();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
}

TEST_F(WebEmbeddedWorkerImplTest, TerminateWhileLoadingScript) {
  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Load the shadow page.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerNetworkProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Terminate before loading the script.
  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(1);
  worker_->TerminateWorkerContext();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
}

TEST_F(WebEmbeddedWorkerImplTest, TerminateWhilePausedAfterDownload) {
  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  start_data_.pause_after_download_mode =
      WebEmbeddedWorkerStartData::kPauseAfterDownload;
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Load the shadow page.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerNetworkProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Load the script.
  EXPECT_CALL(*mock_client_, WorkerScriptLoaded()).Times(1);
  EXPECT_CALL(*mock_client_, CreateServiceWorkerProviderProxy()).Times(0);
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Terminate before resuming after download.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerProviderProxy()).Times(0);
  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(1);
  worker_->TerminateWorkerContext();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
}

TEST_F(WebEmbeddedWorkerImplTest, ScriptNotFound) {
  WebURL script_url =
      URLTestHelpers::ToKURL("https://www.example.com/sw-404.js");
  WebURLResponse response;
  response.SetMIMEType("text/javascript");
  response.SetHTTPStatusCode(404);
  ResourceError error = ResourceError::Failure(script_url);
  Platform::Current()->GetURLLoaderMockFactory()->RegisterErrorURL(
      script_url, response, error);
  start_data_.script_url = script_url;

  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Load the shadow page.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerNetworkProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Load the script.
  EXPECT_CALL(*mock_client_, WorkerScriptLoaded()).Times(0);
  EXPECT_CALL(*mock_client_, CreateServiceWorkerProviderProxy()).Times(0);
  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(1);
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
}

TEST_F(WebEmbeddedWorkerImplTest, DontPauseAfterDownload) {
  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Load the shadow page.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerNetworkProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Load the script.
  EXPECT_CALL(*mock_client_, WorkerScriptLoaded()).Times(1);
  EXPECT_CALL(*mock_client_, CreateServiceWorkerProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  // This is called on the worker thread.
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  mock_client_->WaitUntilScriptEvaluated();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Terminate the running worker thread.
  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(0);
  worker_->TerminateWorkerContext();
  mock_client_->WaitUntilThreadTermination();
}

TEST_F(WebEmbeddedWorkerImplTest, PauseAfterDownload) {
  EXPECT_CALL(*mock_client_, WorkerReadyForInspection()).Times(1);
  start_data_.pause_after_download_mode =
      WebEmbeddedWorkerStartData::kPauseAfterDownload;
  worker_->StartWorkerContext(start_data_);
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Load the shadow page.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerNetworkProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Load the script.
  EXPECT_CALL(*mock_client_, WorkerScriptLoaded()).Times(1);
  EXPECT_CALL(*mock_client_, CreateServiceWorkerProviderProxy()).Times(0);
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);

  // Resume after download.
  EXPECT_CALL(*mock_client_, CreateServiceWorkerProviderProxy())
      .WillOnce(::testing::Return(nullptr));
  // This is called on the worker thread.
  if (mock_installed_scripts_manager_) {
    EXPECT_CALL(*mock_installed_scripts_manager_,
                IsScriptInstalled(start_data_.script_url))
        .Times(1)
        .WillOnce(::testing::Return(false));
  }
  worker_->ResumeAfterDownload();
  mock_client_->WaitUntilScriptEvaluated();
  ::testing::Mock::VerifyAndClearExpectations(mock_client_);
  if (mock_installed_scripts_manager_) {
    ::testing::Mock::VerifyAndClearExpectations(
        mock_installed_scripts_manager_);
  }

  // Terminate the running worker thread.
  EXPECT_CALL(*mock_client_, WorkerContextFailedToStart()).Times(0);
  worker_->TerminateWorkerContext();
  mock_client_->WaitUntilThreadTermination();
}

}  // namespace blink
