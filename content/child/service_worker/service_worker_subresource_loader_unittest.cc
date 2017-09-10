// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_subresource_loader.h"

#include "base/run_loop.h"
#include "content/child/child_url_loader_factory_getter_impl.h"
#include "content/child/service_worker/controller_service_worker_connector.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_url_loader_client.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class FakeControllerServiceWorker : public mojom::ControllerServiceWorker {
 public:
  FakeControllerServiceWorker() = default;
  ~FakeControllerServiceWorker() override = default;

  void AddBinding(mojom::ControllerServiceWorker request) {
    bindings_.AddBinding(this, std::move(request));
  }

  void CloseAllBindings() { bindings_.CloseAllBindings(); }

  // mojom::ControllerServiceWorker:
  void DispatchFetchEvent(
      int fetch_event_id,
      const ServiceWorkerFetchRequest& request,
      mojom::FetchEventPreloadHandlePtr preload_handle,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
      DispatchFetchEventCallback callback) override {
    fetch_event_count_++;
    fetch_event_request_ = request;
    std::move(callback).Run(SERVICE_WORKER_OK, base::Time());
  }

  int fetch_event_count() const { return fetch_event_count_; }
  const ServiceWorkerFetchRequest& fetch_event_request() const {
    return fetch_event_request_;
  }

 private:
  int fetch_event_count_ = 0;
  ServiceWorkerFetchRequest fetch_event_request_;
  mojo::BindingSet<mojom::ControllerServiceWorker> bindings_;
  DISALLOW_COPY_AND_ASSIGN(FakeControllerServiceWorker);
};

class FakeServiceWorkerContainerHost
    : public mojom::ServiceWorkerContainerHost {
 public:
  explicit FakeServiceWorkerContainerHost(
      FakeControllerServiceWorker* fake_controller)
      : fake_controller_(fake_controller) {}

  ~FakeServiceWorkerContainerHost() override = default;

 private:
  // Implements mojom::ServiceWorkerContainerHost.
  void Register(const GURL& script_url,
                const ServiceWorkerRegistrationOptions& options,
                RegisterCallback callback) override {
    NOTIMPLEMENTED();
  }
  void GetRegistration(const GURL& client_url,
                       GetRegistrationCallback callback) override {
    NOTIMPLEMENTED();
  }
  void GetRegistrations(GetRegistrationsCallback callback) override {
    NOTIMPLEMENTED();
  }
  void GetControllerServiceWorker(
      mojom::ControllerServiceWorkerRequest request) override {
    fake_controller_->Addbinding(std::move(request));
  }

  FakeControllerServiceWorker* fake_controller_;
  DISALLOW_COPY_AND_ASSIGN(FakeServiceWorkerContainerHost);
};

}  // namespace

class ServiceWorkerSubresourceLoaderTest : public ::testing::Test {
 public:
  ServiceWorkerSubresourceLoaderTest()
      : fake_container_host_(&fake_controller_) {}
  ~ServiceWorkerSubresourceLoaderTest() override = default;

  void TestRequest(const GURL& url, const std::string& method) {
    ResourceRequest request;
    request.url = url;
    request.method = method;

    auto loader_factory_getter =
        base::MakeRefCounted<ChildURLLoaderFactoryGetterImpl>();

    mojom::URLLoaderPtr url_loader;
    TestURLLoaderClient url_loader_client;

    EXPECT_EQ(0, fake_controller_.fetch_event_count());

    ServiceWorkerSubresourceLoaderFactory loader_factory(
        base::MakeRefCounted<ControllerServiceWorkerConnector>(
            &fake_container_host_),
        loader_factory_getter, request.url.GetOrigin(),
        base::MakeRefCounted<
            base::RefCountedData<storage::mojom::BlobRegistryPtr>>());
    loader_factory.CreateLoaderAndStart(
        mojo::MakeRequest(&url_loader), 0, 0, mojom::kURLLoadOptionNone,
        request, url_loader_client.CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();

    EXPECT_EQ(1, fake_controller_.fetch_event_count());
    EXPECT_EQ(request.url, fake_controller_.fetch_event_request().url);
    EXPECT_EQ(request.method, fake_controller_.fetch_event_request().method);
  }

  TestBrowserThreadBundle thread_bundle_;
  FakeServiceWorkerContainerHost fake_container_host_;
  FakeControllerServiceWorker fake_controller_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerSubresourceLoaderTest);
};

TEST_F(ServiceWorkerSubresourceLoaderTest, Basic) {
  TestRequest(GURL("https://www.example.com/foo.html"), "GET");
}

// TODO(kinuko): Add more tests.

}  // namespace content
