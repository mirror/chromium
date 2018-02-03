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
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/edk/embedder/embedder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/common/service_worker/service_worker.mojom.h"
#include "third_party/WebKit/common/service_worker/service_worker_registration.mojom.h"
#include "third_party/WebKit/common/service_worker/service_worker_state.mojom.h"

namespace content {
namespace service_worker_handle_unittest {

const int kRenderFrameId = 44;  // A dummy ID for testing.

class MockServiceWorkerObject : public blink::mojom::ServiceWorkerObject {
 public:
  explicit MockServiceWorkerObject(
      blink::mojom::ServiceWorkerObjectInfoPtr info)
      : info_(std::move(info)),
        state_(info_->state),
        binding_(this, std::move(info_->request)) {}
  ~MockServiceWorkerObject() override = default;

  blink::mojom::ServiceWorkerState state() const { return state_; }

 private:
  // Implements blink::mojom::ServiceWorkerObject.
  void StateChanged(blink::mojom::ServiceWorkerState state) override {
    state_ = state;
  }

  blink::mojom::ServiceWorkerObjectInfoPtr info_;
  blink::mojom::ServiceWorkerState state_;
  mojo::AssociatedBinding<blink::mojom::ServiceWorkerObject> binding_;
};

class ServiceWorkerHandleTest : public testing::Test {
 public:
  ServiceWorkerHandleTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}

  void SetUp() override {
    mojo::edk::SetDefaultProcessErrorCallback(base::BindRepeating(
        &ServiceWorkerHandleTest::OnMojoError, base::Unretained(this)));
    helper_.reset(new EmbeddedWorkerTestHelper(base::FilePath()));

    dispatcher_host_ = new ServiceWorkerDispatcherHost(
        helper_->mock_render_process_id(), &resource_context_);
    helper_->RegisterDispatcherHost(helper_->mock_render_process_id(),
                                    dispatcher_host_);
    dispatcher_host_->Init(helper_->context_wrapper());

    const GURL kScope("https://www.example.com/");
    const GURL kScriptUrl("https://www.example.com/sw.js");
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = kScope;
    registration_ = new ServiceWorkerRegistration(
        options, 1L, helper_->context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(registration_.get(), kScriptUrl, 1L,
                                        helper_->context()->AsWeakPtr());
    registration_->SetInstallingVersion(version_);
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
    provider_host_->SetDocumentUrl(kScope);
  }

  void TearDown() override {
    mojo::edk::SetDefaultProcessErrorCallback(
        mojo::edk::ProcessErrorCallback());
    dispatcher_host_ = nullptr;
    registration_ = nullptr;
    version_ = nullptr;
    provider_host_.reset();
    helper_.reset();
  }

  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr
  GetRegistrationFromRemote(const GURL& scope) {
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr registration_info;
    mojom::ServiceWorkerContainerHost* container_host =
        remote_endpoint_.host_ptr()->get();
    container_host->GetRegistration(
        scope, base::BindOnce(
                   [](blink::mojom::ServiceWorkerRegistrationObjectInfoPtr*
                          out_registration_info,
                      blink::mojom::ServiceWorkerErrorType error,
                      const base::Optional<std::string>& error_msg,
                      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr
                          registration) {
                     ASSERT_EQ(blink::mojom::ServiceWorkerErrorType::kNone,
                               error);
                     *out_registration_info = std::move(registration);
                   },
                   &registration_info));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(registration_info);
    return registration_info;
  }

  void OnMojoError(const std::string& error) { bad_messages_.push_back(error); }

  std::vector<std::string> bad_messages_;

  TestBrowserThreadBundle browser_thread_bundle_;
  MockResourceContext resource_context_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  std::unique_ptr<ServiceWorkerProviderHost> provider_host_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  scoped_refptr<ServiceWorkerDispatcherHost> dispatcher_host_;
  ServiceWorkerRemoteProviderEndpoint remote_endpoint_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerHandleTest);
};

TEST_F(ServiceWorkerHandleTest, OnVersionStateChanged) {
  const GURL kScope("https://www.example.com/");
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr registration =
      GetRegistrationFromRemote(kScope);
  // |version_| is already on INSTALLING state due to SetUp().
  EXPECT_TRUE(registration->installing);
  auto mock_object = std::make_unique<MockServiceWorkerObject>(
      std::move(registration->installing));
  EXPECT_EQ(blink::mojom::ServiceWorkerState::kInstalling,
            mock_object->state());

  // Start the worker, and then...
  ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
  version_->StartWorker(ServiceWorkerMetrics::EventType::UNKNOWN,
                        CreateReceiverOnCurrentThread(&status));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SERVICE_WORKER_OK, status);
  EXPECT_EQ(blink::mojom::ServiceWorkerState::kInstalling,
            mock_object->state());

  // ...update state to installed.
  version_->SetStatus(ServiceWorkerVersion::INSTALLED);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(bad_messages_.empty());
  // StateChanged (state == Installed).
  EXPECT_EQ(blink::mojom::ServiceWorkerState::kInstalled, mock_object->state());
}

}  // namespace service_worker_handle_unittest
}  // namespace content
