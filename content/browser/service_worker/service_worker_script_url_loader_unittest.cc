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
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace content {

namespace {

class MockURLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  MockURLLoaderFactory() = default;

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    std::string body("// Hello, world!");
    const char header[] =
        "HTTP/1.1 200 OK\n"
        "Content-Type: application/javascript\n"
        "\n";

    net::HttpResponseInfo info;
    info.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        std::string(header, arraysize(header)));
    ResourceResponseHead response;
    response.headers = info.headers;
    response.headers->GetMimeType(&response.mime_type);
    client->OnReceiveResponse(response, base::nullopt, nullptr);

    uint32_t bytes_written = body.size();
    mojo::DataPipe data_pipe;
    data_pipe.producer_handle->WriteData(body.data(), &bytes_written,
                                         MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
    client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));
    client->OnComplete(ResourceRequestCompletionStatus());
  }

  void Clone(mojom::URLLoaderFactoryRequest factory) override { NOTREACHED(); }

 private:
  void OnConnectionError() { delete this; }

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
    helper_ = base::MakeUnique<EmbeddedWorkerTestHelper>(
        base::FilePath(), base::MakeRefCounted<URLLoaderFactoryGetter>());

    // Initialize URLLoaderFactory.
    mojom::URLLoaderFactoryPtr test_loader_factory;
    mojo::MakeStrongBinding(base::MakeUnique<MockURLLoaderFactory>(),
                            MakeRequest(&test_loader_factory));
    helper_->url_loader_factory_getter()->SetNetworkFactoryForTesting(
        std::move(test_loader_factory));

    // Set up ServiceWorkerRegistration and ServiceWorkerVersion that is
    // now starting up.
    GURL scope("https://www.example.com/");
    GURL script_url("https://example.com/sw.js");
    registration_ = base::MakeRefCounted<ServiceWorkerRegistration>(
        ServiceWorkerRegistrationOptions(scope), 1L,
        helper_->context()->AsWeakPtr());
    version_ = base::MakeRefCounted<ServiceWorkerVersion>(
        registration_.get(), script_url, 1L, helper_->context()->AsWeakPtr());
    version_->SetStatus(ServiceWorkerVersion::NEW);
  }

  void Request() {
    // Dummy values.
    int routing_id = 0;
    int request_id = 10;
    uint32_t options = 0;

    ResourceRequest request;
    request.url = version_->script_url();
    request.method = "GET";

    DCHECK(!loader_);
    loader_ = base::MakeUnique<ServiceWorkerScriptURLLoader>(
        routing_id, request_id, options, request, client_.CreateInterfacePtr(),
        version_, helper_->url_loader_factory_getter(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;

  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;

  TestURLLoaderClient client_;
  std::unique_ptr<ServiceWorkerScriptURLLoader> loader_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
};

TEST_F(ServiceWorkerScriptURLLoaderTest, Success) {
  Request();
  client_.RunUntilComplete();

  EXPECT_TRUE(client_.has_received_response());
  // TODO(nhiroki): Check the received response.
  EXPECT_TRUE(client_.has_received_completion());
  EXPECT_EQ(net::OK, client_.completion_status().error_code);
}

TEST_F(ServiceWorkerScriptURLLoaderTest, RedundantWorker) {
  Request();

  // Makes the service worker redundant.
  version_->Doom();
  ASSERT_TRUE(version_->is_redundant());

  client_.RunUntilComplete();

  // The request should be aborted.
  EXPECT_FALSE(client_.has_received_response());
  EXPECT_TRUE(client_.has_received_completion());
  EXPECT_EQ(net::ERR_FAILED, client_.completion_status().error_code);
}

}  // namespace content
