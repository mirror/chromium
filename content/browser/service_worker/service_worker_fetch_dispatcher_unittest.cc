// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_fetch_dispatcher.h"

#include "base/run_loop.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/http/http_util.h"

namespace content {

class MockURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  static void Create(mojom::URLLoaderFactoryPtr* loader_factory) {
    mojom::URLLoaderFactoryRequest request = mojo::MakeRequest(loader_factory);
    new MockURLLoaderFactory(std::move(request));
  }

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    std::string headers = "HTTP/1.1 200 OK\n\n";
    net::HttpResponseInfo info;
    info.headers = new net::HttpResponseHeaders(
        net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.length()));
    ResourceResponseHead response;
    response.headers = info.headers;
    response.headers->GetMimeType(&response.mime_type);
    client->OnReceiveResponse(response, base::nullopt, nullptr);

    std::string body = "this is the body";
    uint32_t bytes_written = body.size();
    mojo::DataPipe data_pipe;
    data_pipe.producer_handle->WriteData(body.data(), &bytes_written,
                                         MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
    client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

    ResourceRequestCompletionStatus status;
    status.error_code = net::OK;
    client->OnComplete(status);
  }

  void Clone(mojom::URLLoaderFactoryRequest factory) override { NOTREACHED(); }

 private:
  MockURLLoaderFactory(mojom::URLLoaderFactoryRequest request)
      : binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindOnce(
        &MockURLLoaderFactory::OnConnectionError, base::Unretained(this)));
  }

  void OnConnectionError() { delete this; }

  mojo::Binding<mojom::URLLoaderFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(MockURLLoaderFactory);
};

class ServiceWorkerFetchDispatcherTest : public testing::Test {
 public:
  ServiceWorkerFetchDispatcherTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        helper_(base::MakeUnique<EmbeddedWorkerTestHelper>(
            base::FilePath(),
            base::MakeRefCounted<URLLoaderFactoryGetter>())) {
    mojom::URLLoaderFactoryPtr mock_loader_factory;
    MockURLLoaderFactory::Create(&mock_loader_factory);
    helper_->url_loader_factory_getter()->SetNetworkFactoryForTesting(
        std::move(mock_loader_factory));
  }
  ~ServiceWorkerFetchDispatcherTest() override = default;

  void SetUp() override {
    // Create an active service worker.
    storage()->LazyInitialize(base::Bind(&base::DoNothing));
    base::RunLoop().RunUntilIdle();
    registration_ = new ServiceWorkerRegistration(
        ServiceWorkerRegistrationOptions(GURL("https://example.com/")),
        storage()->NewRegistrationId(), helper_->context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(
        registration_.get(), GURL("https://example.com/service_worker.js"),
        storage()->NewVersionId(), helper_->context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(ServiceWorkerDatabase::ResourceRecord(
        storage()->NewResourceId(), version_->script_url(), 100));
    version_->script_cache_map()->SetResources(records);
    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
    registration_->SetActiveVersion(version_);

    // Make the registration findable via storage functions.
    registration_->set_last_update_check(base::Time::Now());
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_FAILED;
    storage()->StoreRegistration(registration_.get(), version_.get(),
                                 CreateReceiverOnCurrentThread(&status));
    base::RunLoop().RunUntilIdle();
    ASSERT_EQ(SERVICE_WORKER_OK, status);

    auto request = base::MakeUnique<ServiceWorkerFetchRequest>(
        GURL("https://www.example.com"), "GET", ServiceWorkerHeaderMap(),
        Referrer(), false /* is_reload */);
    fetch_dispatcher_ = base::MakeUnique<ServiceWorkerFetchDispatcher>(
        std::move(request), version_.get(), RESOURCE_TYPE_MAIN_FRAME,
        base::nullopt /* timeout */, net::NetLogWithSource(),
        base::Bind(&base::DoNothing),
        base::Bind(&ServiceWorkerFetchDispatcherTest::DidDispatchFetchEvent,
                   base::Unretained(this)));
  }

  ServiceWorkerStorage* storage() { return helper_->context()->storage(); }

  void DidDispatchFetchEvent(
      ServiceWorkerStatusCode status,
      ServiceWorkerFetchEventResult fetch_result,
      const ServiceWorkerResponse& response,
      blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
      storage::mojom::BlobPtr body_as_blob,
      const scoped_refptr<ServiceWorkerVersion>& version) {
    status_ = status;
    fetch_result_ = fetch_result;
    response_ = response;
    body_as_stream_ = std::move(body_as_stream);
    body_as_blob_ = std::move(body_as_blob);
    EXPECT_EQ(version_, version);
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
  std::unique_ptr<ServiceWorkerFetchDispatcher> fetch_dispatcher_;
  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;

  // Filled after DidDispatchFetchEvent.
  ServiceWorkerStatusCode status_;
  ServiceWorkerFetchEventResult fetch_result_;
  ServiceWorkerResponse response_;
  blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream_;
  storage::mojom::BlobPtr body_as_blob_;
};

TEST_F(ServiceWorkerFetchDispatcherTest, Basic) {
  fetch_dispatcher_->Run();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(SERVICE_WORKER_OK, status_);
}

}  // namespace content
