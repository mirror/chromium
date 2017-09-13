// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_script_url_loader.h"

#include "base/run_loop.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/common/resource_request_completion_status.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_url_loader_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace content {

namespace {

const char kDummyHeaders[] = "HTTP/1.1 200 OK\n\n";
const char kDummyBody[] = "this body came from the network";

// A URLLoaderFactory that returns 200 OK with a simple body to any request.
//
// TODO(nhiroki): We copied this from service_worker_url_loader_job_unittest.cc
// instead of making it a common test helper because we might want to customize
// the mock factory to add more tests later. Merge this and that if we're
// convinced it's better.
class MockNetworkURLLoaderFactory final : public mojom::URLLoaderFactory {
 public:
  MockNetworkURLLoaderFactory() = default;

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    std::string headers(kDummyHeaders, arraysize(kDummyHeaders));
    net::HttpResponseInfo info;
    info.headers =
        new net::HttpResponseHeaders(net::HttpUtil::AssembleRawHeaders(
            kDummyHeaders, arraysize(kDummyHeaders)));
    ResourceResponseHead response;
    response.headers = info.headers;
    response.headers->GetMimeType(&response.mime_type);
    client->OnReceiveResponse(response, base::nullopt, nullptr);

    std::string body(kDummyBody, arraysize(kDummyBody));
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
  DISALLOW_COPY_AND_ASSIGN(MockNetworkURLLoaderFactory);
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

    InitializeStorage();

    // Initialize URLLoaderFactory.
    mojom::URLLoaderFactoryPtr test_loader_factory;
    mojo::MakeStrongBinding(base::MakeUnique<MockNetworkURLLoaderFactory>(),
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

  void InitializeStorage() {
    base::RunLoop run_loop;
    helper_->context()->storage()->LazyInitializeForTest(
        run_loop.QuitClosure());
    run_loop.Run();
  }

  void DoRequest() {
    // Dummy values.
    int routing_id = 0;
    int request_id = 10;
    uint32_t options = 0;

    ResourceRequest request;
    request.url = version_->script_url();
    request.method = "GET";
    request.resource_type = RESOURCE_TYPE_SERVICE_WORKER;

    DCHECK(!loader_);
    loader_ = base::MakeUnique<ServiceWorkerScriptURLLoader>(
        routing_id, request_id, options, request, client_.CreateInterfacePtr(),
        version_, helper_->url_loader_factory_getter(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  void VerifyStoredResponse(int64_t resource_id) {
    // Verify the response status.
    {
      auto info_buffer = base::MakeRefCounted<HttpResponseInfoIOBuffer>();
      std::unique_ptr<ServiceWorkerResponseReader> reader =
          helper_->context()->storage()->CreateResponseReader(resource_id);
      net::TestCompletionCallback cb;
      reader->ReadInfo(info_buffer.get(), cb.callback());
      int rv = cb.WaitForResult();
      EXPECT_LT(0, rv);
      EXPECT_EQ("OK", info_buffer->http_info->headers->GetStatusText());
    }

    // Verify the response body.
    {
      const std::string kExpectedHttpBody(kDummyBody, arraysize(kDummyBody));
      std::unique_ptr<ServiceWorkerResponseReader> reader =
          helper_->context()->storage()->CreateResponseReader(resource_id);

      std::string received_body;
      const int kBigEnough = 512;
      auto buffer = base::MakeRefCounted<net::IOBuffer>(kBigEnough);
      net::TestCompletionCallback cb;
      reader->ReadData(buffer.get(), kBigEnough, cb.callback());
      int rv = cb.WaitForResult();
      EXPECT_EQ(static_cast<int>(kExpectedHttpBody.size()), rv);
      EXPECT_LT(0, rv);
      received_body.assign(buffer->data(), rv);

      EXPECT_EQ(kExpectedHttpBody, received_body);
    }
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;

  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  std::unique_ptr<ServiceWorkerScriptURLLoader> loader_;

  TestURLLoaderClient client_;
};

TEST_F(ServiceWorkerScriptURLLoaderTest, Success) {
  DoRequest();
  client_.RunUntilComplete();

  EXPECT_TRUE(client_.has_received_response());

  int64_t resource_id = version_->script_cache_map()->LookupResourceId(
      GURL("https://example.com/sw.js"));
  VerifyStoredResponse(resource_id);

  // TODO(nhiroki): Check the received response.
  EXPECT_EQ(net::OK, client_.completion_status().error_code);
}

TEST_F(ServiceWorkerScriptURLLoaderTest, RedundantWorker) {
  DoRequest();

  // Make the service worker redundant.
  version_->Doom();
  ASSERT_TRUE(version_->is_redundant());

  client_.RunUntilComplete();

  // The request should be aborted.
  EXPECT_FALSE(client_.has_received_response());
  EXPECT_EQ(net::ERR_FAILED, client_.completion_status().error_code);
}

}  // namespace content
