// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_script_url_loader.h"

#include "base/run_loop.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/resource_request_completion_status.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_url_loader_client.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace content {

namespace {

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
    client_ = std::move(client);

    // Post a task to emulate an async resource fetch.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&MockURLLoaderFactory::OnReceiveResponse,
                       base::Unretained(this)));
  }

  void OnReceiveResponse() {
    std::string body("// Hello, world!");
    const char data[] =
        "HTTP/1.1 200 OK\0"
        "Content-Type: application/javascript\0"
        "\0";

    net::HttpResponseInfo info;
    info.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        std::string(data, arraysize(data)));

    ResourceResponseHead response;
    response.headers = info.headers;
    response.headers->GetMimeType(&response.mime_type);

    client_->OnReceiveResponse(response, base::nullopt, nullptr);

    mojo::DataPipe data_pipe;

    uint32_t bytes_written = body.size();
    data_pipe.producer_handle->WriteData(body.data(), &bytes_written,
                                         MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
    client_->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

    // Post a task to emulate an async resource fetch.
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::BindOnce(&MockURLLoaderFactory::OnComplete,
                                           base::Unretained(this)));
  }

  void OnComplete() { client_->OnComplete(ResourceRequestCompletionStatus()); }

  void Clone(mojom::URLLoaderFactoryRequest factory) override { NOTREACHED(); }

 private:
  MockURLLoaderFactory(mojom::URLLoaderFactoryRequest request)
      : binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindOnce(
        &MockURLLoaderFactory::OnConnectionError, base::Unretained(this)));
  }

  void OnConnectionError() { delete this; }

  mojo::Binding<mojom::URLLoaderFactory> binding_;
  mojom::URLLoaderClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(MockURLLoaderFactory);
};

}  // namespace

// ServiceWorkerScriptURLLoaderTest is for testing the handling of requests for
// installing service worker scripts via ServiceWorkerScriptURLLoader.
class ServiceWorkerScriptURLLoaderTest : public testing::Test {
 public:
  ServiceWorkerScriptURLLoaderTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}
  ~ServiceWorkerScriptURLLoaderTest() override = default;

  void SetUp() override {
    helper_ = base::MakeUnique<EmbeddedWorkerTestHelper>(base::FilePath());

    loader_factory_getter_ = base::MakeRefCounted<URLLoaderFactoryGetter>();
    InitializeFactory();

    GURL scope("https://www.example.com/");
    GURL script_url("https://example.com/service_worker.js");
    registration_ = base::MakeRefCounted<ServiceWorkerRegistration>(
        ServiceWorkerRegistrationOptions(scope), 1L,
        helper_->context()->AsWeakPtr());
    version_ = base::MakeRefCounted<ServiceWorkerVersion>(
        registration_.get(), script_url, 1L, helper_->context()->AsWeakPtr());
    version_->SetStatus(ServiceWorkerVersion::NEW);
  }

  void Request() {
    int routing_id = 0;
    int request_id = 10;
    uint32_t options = 0;

    ResourceRequest request;
    request.url = GURL("https://www.example.com/service_worker.js");
    request.method = "GET";

    loader_ = base::MakeUnique<ServiceWorkerScriptURLLoader>(
        routing_id, request_id, options, request, client_.CreateInterfacePtr(),
        version_, loader_factory_getter_,
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  void RunUntilComplete() { client_.RunUntilComplete(); }

  // TODO: Copied from appcache_update_job_unittest.cc
  void InitializeFactory() {
    mojom::URLLoaderFactoryPtr test_loader_factory;
    MockURLLoaderFactory::Create(&test_loader_factory);
    loader_factory_getter_->SetNetworkFactoryForTesting(
        std::move(test_loader_factory));
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;

  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;

  TestURLLoaderClient client_;
  std::unique_ptr<ServiceWorkerScriptURLLoader> loader_;
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
};

TEST_F(ServiceWorkerScriptURLLoaderTest, Basic) {
  Request();
  RunUntilComplete();
  EXPECT_EQ(net::OK, client_.completion_status().error_code);
}

TEST_F(ServiceWorkerScriptURLLoaderTest, Redundant) {
  Request();
  version_->Doom();
  ASSERT_TRUE(version_->is_redundant());
  client_.RunUntilComplete();

  EXPECT_FALSE(client_.has_received_response());
  EXPECT_TRUE(client_.has_received_completion());
  EXPECT_EQ(net::ERR_FAILED, client_.completion_status().error_code);
  client_.Unbind();
}

}  // namespace content
