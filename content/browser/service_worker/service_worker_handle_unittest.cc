// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>
#include <vector>

#include "base/macros.h"
#include "base/run_loop.h"
#include "content/browser/service_worker/embedded_worker_registry.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_dispatcher_host.h"
#include "content/browser/service_worker/service_worker_handle.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_test_utils.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/embedded_worker_messages.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/common/service_worker/service_worker_registration.mojom.h"
#include "third_party/WebKit/common/service_worker/service_worker_state.mojom.h"

namespace content {
namespace service_worker_handle_unittest {

const int kRenderFrameId = 44;  // A dummy ID for testing.

void VerifyStateChangedMessage(int expected_handle_id,
                               blink::mojom::ServiceWorkerState expected_state,
                               const IPC::Message* message) {
  ASSERT_TRUE(message != nullptr);
  ServiceWorkerMsg_ServiceWorkerStateChanged::Param param;
  ASSERT_TRUE(ServiceWorkerMsg_ServiceWorkerStateChanged::Read(
      message, &param));
  EXPECT_EQ(expected_handle_id, std::get<1>(param));
  EXPECT_EQ(expected_state, std::get<2>(param));
}

class TestingServiceWorkerDispatcherHost : public ServiceWorkerDispatcherHost {
 public:
  TestingServiceWorkerDispatcherHost(int process_id,
                                     ResourceContext* resource_context,
                                     EmbeddedWorkerTestHelper* helper)
      : ServiceWorkerDispatcherHost(process_id, resource_context),
        bad_message_received_count_(0),
        helper_(helper) {}

  bool Send(IPC::Message* message) override { return helper_->Send(message); }

  void ShutdownForBadMessage() override { ++bad_message_received_count_; }

  int bad_message_received_count_;

 protected:
  EmbeddedWorkerTestHelper* helper_;
  ~TestingServiceWorkerDispatcherHost() override {}
};

class ServiceWorkerHandleTest : public testing::Test {
 public:
  ServiceWorkerHandleTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    helper_.reset(new EmbeddedWorkerTestHelper(base::FilePath()));

    dispatcher_host_ = new TestingServiceWorkerDispatcherHost(
        helper_->mock_render_process_id(), &resource_context_, helper_.get());
    helper_->RegisterDispatcherHost(helper_->mock_render_process_id(),
                                    dispatcher_host_);
    dispatcher_host_->Init(helper_->context_wrapper());

    const GURL pattern("http://www.example.com/");
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = pattern;
    registration_ = new ServiceWorkerRegistration(
        options, 1L, helper_->context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(
        registration_.get(),
        GURL("http://www.example.com/service_worker.js"),
        1L,
        helper_->context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(
        ServiceWorkerDatabase::ResourceRecord(10, version_->script_url(), 100));
    version_->script_cache_map()->SetResources(records);
    version_->SetMainScriptHttpResponseInfo(net::HttpResponseInfo());
    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    version_->SetStatus(ServiceWorkerVersion::INSTALLING);

    // Make the registration findable via storage functions.
    helper_->context()->storage()->LazyInitializeForTest(
        base::BindOnce(&base::DoNothing));
    base::RunLoop().RunUntilIdle();
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
    helper_->context()->storage()->StoreRegistration(
        registration_.get(),
        version_.get(),
        CreateReceiverOnCurrentThread(&status));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(SERVICE_WORKER_OK, status);

    provider_host_ = CreateProviderHostWithDispatcherHost(
        helper_->mock_render_process_id(), 1 /* provider_id */,
        helper_->context()->AsWeakPtr(), kRenderFrameId, dispatcher_host_.get(),
        &remote_endpoint_);
  }

  void TearDown() override {
    dispatcher_host_ = nullptr;
    registration_ = nullptr;
    version_ = nullptr;
    provider_host_.reset();
    helper_.reset();
  }

  IPC::TestSink* ipc_sink() { return helper_->ipc_sink(); }

  TestBrowserThreadBundle browser_thread_bundle_;
  MockResourceContext resource_context_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  std::unique_ptr<ServiceWorkerProviderHost> provider_host_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  scoped_refptr<TestingServiceWorkerDispatcherHost> dispatcher_host_;
  ServiceWorkerRemoteProviderEndpoint remote_endpoint_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerHandleTest);
};

TEST_F(ServiceWorkerHandleTest, OnVersionStateChanged) {
  blink::mojom::ServiceWorkerObjectInfoPtr info;
  // ServiceWorkerHandle lifetime is controlled by |info| and is also owned by
  // |dispatcher_host_|.
  auto handle = ServiceWorkerHandle::Create(
      dispatcher_host_.get(), helper_->context()->AsWeakPtr(),
      provider_host_->AsWeakPtr(), version_.get(), &info);

  // Start the worker, and then...
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
  version_->StartWorker(ServiceWorkerMetrics::EventType::UNKNOWN,
                        CreateReceiverOnCurrentThread(&status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SERVICE_WORKER_OK, status);

  // ...update state to installed.
  version_->SetStatus(ServiceWorkerVersion::INSTALLED);

  ASSERT_EQ(0L, dispatcher_host_->bad_message_received_count_);

  const IPC::Message* message = nullptr;
  // StartWorker shouldn't be recorded here.
  ASSERT_EQ(1UL, ipc_sink()->message_count());
  message = ipc_sink()->GetMessageAt(0);

  // StateChanged (state == Installed).
  VerifyStateChangedMessage(handle->handle_id(),
                            blink::mojom::ServiceWorkerState::kInstalled,
                            message);
}

}  // namespace service_worker_handle_unittest
}  // namespace content
