// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_provider_context.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "content/child/service_worker/service_worker_dispatcher.h"
#include "content/child/service_worker/service_worker_handle_reference.h"
#include "content/child/service_worker/service_worker_provider_context.h"
#include "content/child/service_worker/service_worker_registration_handle_reference.h"
#include "content/child/service_worker/web_service_worker_impl.h"
#include "content/child/service_worker/web_service_worker_registration_impl.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/service_worker/service_worker_container.mojom.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/child/child_url_loader_factory_getter.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "ipc/ipc_sync_message_filter.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerProviderClient.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_registration.mojom.h"

namespace content {

namespace {

class ServiceWorkerTestSender : public ThreadSafeSender {
 public:
  explicit ServiceWorkerTestSender(IPC::TestSink* ipc_sink)
      : ThreadSafeSender(nullptr, nullptr), ipc_sink_(ipc_sink) {}

  bool Send(IPC::Message* message) override { return ipc_sink_->Send(message); }

 private:
  ~ServiceWorkerTestSender() override {}

  IPC::TestSink* ipc_sink_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerTestSender);
};

}  // namespace

class ServiceWorkerProviderContextTest : public testing::Test {
 public:
  ServiceWorkerProviderContextTest() = default;

  void SetUp() override {
    sender_ = new ServiceWorkerTestSender(&ipc_sink_);
    dispatcher_.reset(new ServiceWorkerDispatcher(sender_.get(), nullptr));
  }

  void CreateObjectInfoAndVersionAttributes(
      blink::mojom::ServiceWorkerRegistrationObjectInfoPtr* info,
      ServiceWorkerVersionAttributes* attrs) {
    *info = blink::mojom::ServiceWorkerRegistrationObjectInfo::New();
    (*info)->handle_id = 10;
    (*info)->registration_id = 20;

    attrs->active.handle_id = 100;
    attrs->active.version_id = 200;
    attrs->waiting.handle_id = 101;
    attrs->waiting.version_id = 201;
    attrs->installing.handle_id = 102;
    attrs->installing.version_id = 202;
  }

  ThreadSafeSender* thread_safe_sender() { return sender_.get(); }
  IPC::TestSink* ipc_sink() { return &ipc_sink_; }
  ServiceWorkerDispatcher* dispatcher() { return dispatcher_.get(); }

 private:
  base::MessageLoop message_loop_;
  IPC::TestSink ipc_sink_;
  std::unique_ptr<ServiceWorkerDispatcher> dispatcher_;
  scoped_refptr<ServiceWorkerTestSender> sender_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerProviderContextTest);
};

class MockedWebServiceWorkerProviderClientImpl
    : public blink::WebServiceWorkerProviderClient {
 public:
  MockedWebServiceWorkerProviderClientImpl() {}

  ~MockedWebServiceWorkerProviderClientImpl() override {}

  void SetController(std::unique_ptr<blink::WebServiceWorker::Handle> handle,
                     bool should_notify_controller_change) override {
    // WebPassOwnPtr cannot be owned in Chromium, so drop the handle here.
    // The destruction releases ServiceWorkerHandleReference.
    is_set_controller_called_ = true;
  }

  void DispatchMessageEvent(
      std::unique_ptr<blink::WebServiceWorker::Handle> handle,
      const blink::WebString& message,
      blink::WebMessagePortChannelArray channels) override {
    // WebPassOwnPtr cannot be owned in Chromium, so drop the handle here.
    // The destruction releases ServiceWorkerHandleReference.
    is_dispatch_message_event_called_ = true;
  }

  void CountFeature(uint32_t feature) override {
    used_features_.insert(feature);
  }

  bool is_set_controller_called() const { return is_set_controller_called_; }

  bool is_dispatch_message_event_called() const {
    return is_dispatch_message_event_called_;
  }

 private:
  bool is_set_controller_called_ = false;
  bool is_dispatch_message_event_called_ = false;
  std::set<uint32_t> used_features_;
};

TEST_F(ServiceWorkerProviderContextTest, CreateForController) {
  // Assume that these objects are passed from the browser process and own
  // references to browser-side registration/worker representations.
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr registration_info;
  ServiceWorkerVersionAttributes version_attrs;
  CreateObjectInfoAndVersionAttributes(&registration_info, &version_attrs);
  std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration =
      ServiceWorkerRegistrationHandleReference::Adopt(
          std::move(registration_info), thread_safe_sender());
  std::unique_ptr<ServiceWorkerHandleReference> installing =
      ServiceWorkerHandleReference::Adopt(version_attrs.installing,
                                          thread_safe_sender());
  std::unique_ptr<ServiceWorkerHandleReference> waiting =
      ServiceWorkerHandleReference::Adopt(version_attrs.waiting,
                                          thread_safe_sender());
  std::unique_ptr<ServiceWorkerHandleReference> active =
      ServiceWorkerHandleReference::Adopt(version_attrs.active,
                                          thread_safe_sender());

  // Set up ServiceWorkerProviderContext for ServiceWorkerGlobalScope.
  const int kProviderId = 10;
  auto provider_context = base::MakeRefCounted<ServiceWorkerProviderContext>(
      kProviderId, SERVICE_WORKER_PROVIDER_FOR_CONTROLLER, nullptr, nullptr,
      dispatcher(), nullptr /* loader_factory_getter */);

  // The passed references should be adopted and owned by the provider context.
  provider_context->SetRegistration(std::move(registration),
                                    std::move(installing), std::move(waiting),
                                    std::move(active));
  EXPECT_EQ(0UL, ipc_sink()->message_count());

  // Destruction of the provider context should release references to the
  // associated registration and its versions.
  provider_context = nullptr;
  ASSERT_EQ(4UL, ipc_sink()->message_count());
  EXPECT_EQ(ServiceWorkerHostMsg_DecrementServiceWorkerRefCount::ID,
            ipc_sink()->GetMessageAt(0)->type());
  EXPECT_EQ(ServiceWorkerHostMsg_DecrementServiceWorkerRefCount::ID,
            ipc_sink()->GetMessageAt(1)->type());
  EXPECT_EQ(ServiceWorkerHostMsg_DecrementServiceWorkerRefCount::ID,
            ipc_sink()->GetMessageAt(2)->type());
  EXPECT_EQ(ServiceWorkerHostMsg_DecrementRegistrationRefCount::ID,
            ipc_sink()->GetMessageAt(3)->type());
}

TEST_F(ServiceWorkerProviderContextTest, SetController) {
  const int kProviderId = 10;

  // Assume that these objects are passed from the browser process and own
  // references to browser-side registration/worker representations.
  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info;
  ServiceWorkerVersionAttributes attrs;
  CreateObjectInfoAndVersionAttributes(&info, &attrs);

  mojom::ServiceWorkerContainerAssociatedRequest container_request;
  // (1) In the case there is no WebSWProviderClient but SWProviderContext for
  // the provider, the passed reference should be adopted and owned by the
  // provider context.
  mojom::ServiceWorkerContainerAssociatedPtr container_ptr1;
  container_request = mojo::MakeIsolatedRequest(&container_ptr1);
  auto provider_context = base::MakeRefCounted<ServiceWorkerProviderContext>(
      kProviderId, SERVICE_WORKER_PROVIDER_FOR_WINDOW,
      std::move(container_request), nullptr /* host_ptr_info */, dispatcher(),
      nullptr /* loader_factory_getter */);
  std::vector<blink::mojom::WebFeature> features;

  ipc_sink()->ClearMessages();
  container_ptr1->SetController(attrs.active, features, true);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0UL, ipc_sink()->message_count());

  // Destruction of the provider context should release references to the
  // associated registration and the controller.
  provider_context = nullptr;
  ASSERT_EQ(1UL, ipc_sink()->message_count());
  EXPECT_EQ(ServiceWorkerHostMsg_DecrementServiceWorkerRefCount::ID,
            ipc_sink()->GetMessageAt(0)->type());
  ipc_sink()->ClearMessages();

  // (2) In the case there are both SWProviderContext and SWProviderClient for
  // the provider, the passed reference should be adopted and owned by the
  // provider context. In addition, the new reference should be created for the
  // provider client and immediately released due to limitation of the mock
  // implementation.
  mojom::ServiceWorkerContainerHostAssociatedPtrInfo host_ptr_info;
  mojom::ServiceWorkerContainerHostAssociatedRequest host_request =
      mojo::MakeRequest(&host_ptr_info);

  mojom::ServiceWorkerContainerAssociatedPtr container_ptr2;
  container_request = mojo::MakeIsolatedRequest(&container_ptr2);
  provider_context = base::MakeRefCounted<ServiceWorkerProviderContext>(
      kProviderId, SERVICE_WORKER_PROVIDER_FOR_WINDOW,
      std::move(container_request), std::move(host_ptr_info), dispatcher(),
      nullptr /* loader_factory_getter */);
  auto provider_impl = base::MakeUnique<WebServiceWorkerProviderImpl>(
      thread_safe_sender(), provider_context.get());
  auto client = base::MakeUnique<MockedWebServiceWorkerProviderClientImpl>();
  provider_impl->SetClient(client.get());
  ASSERT_FALSE(client->is_set_controller_called());

  ipc_sink()->ClearMessages();
  container_ptr2->SetController(attrs.active, features, true);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(client->is_set_controller_called());
  ASSERT_EQ(2UL, ipc_sink()->message_count());
  EXPECT_EQ(ServiceWorkerHostMsg_IncrementServiceWorkerRefCount::ID,
            ipc_sink()->GetMessageAt(0)->type());
  EXPECT_EQ(ServiceWorkerHostMsg_DecrementServiceWorkerRefCount::ID,
            ipc_sink()->GetMessageAt(1)->type());
}

// Test that clearing the controller by sending a kInvalidServiceWorkerHandle
// results in the provider context having a null controller.
TEST_F(ServiceWorkerProviderContextTest, SetController_Null) {
  const int kProviderId = 10;

  blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info;
  ServiceWorkerVersionAttributes attrs;
  CreateObjectInfoAndVersionAttributes(&info, &attrs);

  mojom::ServiceWorkerContainerHostAssociatedPtrInfo host_ptr_info;
  mojom::ServiceWorkerContainerHostAssociatedRequest host_request =
      mojo::MakeRequest(&host_ptr_info);

  mojom::ServiceWorkerContainerAssociatedPtr container_ptr;
  mojom::ServiceWorkerContainerAssociatedRequest container_request =
      mojo::MakeIsolatedRequest(&container_ptr);
  auto provider_context = base::MakeRefCounted<ServiceWorkerProviderContext>(
      kProviderId, SERVICE_WORKER_PROVIDER_FOR_WINDOW,
      std::move(container_request), std::move(host_ptr_info), dispatcher(),
      nullptr /* loader_factory_getter */);
  auto provider_impl = base::MakeUnique<WebServiceWorkerProviderImpl>(
      thread_safe_sender(), provider_context.get());
  auto controllee =
      base::MakeUnique<MockedWebServiceWorkerProviderClientImpl>();
  provider_impl->SetClient(controllee.get());

  container_ptr->SetController(ServiceWorkerObjectInfo(),
                               std::vector<blink::mojom::WebFeature>(), true);
  base::RunLoop().RunUntilIdle();

  // Check that it became null.
  EXPECT_EQ(nullptr, provider_context->controller());
  EXPECT_TRUE(controllee->is_set_controller_called());
}

}  // namespace content
