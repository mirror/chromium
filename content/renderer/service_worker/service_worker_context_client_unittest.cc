// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_context_client.h"

#include "base/threading/thread.h"
#include "content/child/thread_safe_sender.h"
#include "content/renderer/service_worker/embedded_worker_instance_client_impl.h"
#include "ipc/ipc_sync_message_filter.h"
#include "ipc/ipc_test_sink.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_registration.mojom.h"

namespace content {

namespace {

struct ContextClientPipes {
  mojom::ServiceWorkerEventDispatcherPtr event_dispatcher;
  mojom::ControllerServiceWorkerPtr controller;
  blink::mojom::ServiceWorkerHostAssociatedRequest service_worker_host_request;
  mojom::EmbeddedWorkerInstanceHostAssociatedRequest instance_host_request;
};

class TestSender : public ThreadSafeSender {
 public:
  explicit TestSender(IPC::TestSink* ipc_sink)
      : ThreadSafeSender(nullptr, nullptr), ipc_sink_(ipc_sink) {}

  bool Send(IPC::Message* message) override { return ipc_sink_->Send(message); }

 private:
  ~TestSender() override = default;

  IPC::TestSink* ipc_sink_;

  DISALLOW_COPY_AND_ASSIGN(TestSender);
};

}  // namespace

class ServiceWorkerContextClientTest : public testing::Test {
 public:
  ServiceWorkerContextClientTest() : io_thread_("io_thread") {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(io_thread_.Start());
    io_task_runner_ = io_thread_.task_runner();
    sender_ = base::MakeRefCounted<TestSender>(&ipc_sink_);
  }

  mojom::ServiceWorkerProviderInfoForStartWorkerPtr CreateProviderInfo() {
    auto info = mojom::ServiceWorkerProviderInfoForStartWorker::New();
    info->registration =
        blink::mojom::ServiceWorkerRegistrationObjectInfo::New();
    info->registration->installing =
        blink::mojom::ServiceWorkerObjectInfo::New();
    info->registration->waiting = blink::mojom::ServiceWorkerObjectInfo::New();
    info->registration->active = blink::mojom::ServiceWorkerObjectInfo::New();
    return info;
  }

  std::unique_ptr<ServiceWorkerContextClient> CreateContextClient(
      ContextClientPipes* out_pipes) {
    auto event_dispatcher_request =
        mojo::MakeRequest(&out_pipes->event_dispatcher);
    auto controller_request = mojo::MakeRequest(&out_pipes->controller);
    blink::mojom::ServiceWorkerHostAssociatedPtr sw_host_ptr;
    out_pipes->service_worker_host_request =
        mojo::MakeRequestAssociatedWithDedicatedPipe(&sw_host_ptr);
    mojom::EmbeddedWorkerInstanceHostAssociatedPtr instance_host_ptr;
    out_pipes->instance_host_request =
        mojo::MakeRequestAssociatedWithDedicatedPipe(&instance_host_ptr);
    EXPECT_TRUE(sender_);
    return std::make_unique<ServiceWorkerContextClient>(
        1, 1, GURL("https://example.com"), GURL("https://example.com/sw.js"),
        false, std::move(event_dispatcher_request),
        std::move(controller_request), sw_host_ptr.PassInterface(),
        instance_host_ptr.PassInterface(), CreateProviderInfo(), nullptr,
        sender_, io_task_runner_);
  }

 private:
  base::MessageLoop message_loop_;
  base::Thread io_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  IPC::TestSink ipc_sink_;
  scoped_refptr<TestSender> sender_;
};

TEST_F(ServiceWorkerContextClientTest, Ping) {
  ContextClientPipes pipes;
  auto context_client = CreateContextClient(&pipes);

  // TODO(shimazu): Call WorkerContextStarted() and insert
  // blink::WebServiceWorkerContextProxy to manipulate the event dispatched to
  // the context client.
  pipes.event_dispatcher->Ping(base::BindOnce(&base::DoNothing));
  base::RunLoop().RunUntilIdle();
}

}  // namespace content
