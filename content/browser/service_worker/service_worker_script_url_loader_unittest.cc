// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_script_url_loader.h"

#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/public/common/resource_request_completion_status.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace blink {

// TODO: Copied from appcache_update_job_unittest.
// Provides a test URLLoaderFactory which serves content using the
// MockHttpServer and RetryRequestTestJob classes.
// TODO(ananta/michaeln). Remove dependencies on URLRequest based
// classes by refactoring the response headers/data into a common class.
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
    if (url_request.url.host() == "failme" ||
        url_request.url.host() == "testme") {
      ResourceRequestCompletionStatus status;
      status.error_code = -100;
      client->OnComplete(status);
      return;
    }

    HttpHeadersRequestTestJob::ValidateExtraHeaders(url_request.headers);

    std::string headers;
    std::string body;
    if (url_request.url == RetryRequestTestJob::kRetryUrl) {
      RetryRequestTestJob::GetResponseForURL(url_request.url, &headers, &body);
    } else {
      MockHttpServer::GetMockResponse(url_request.url.path(), &headers, &body);
    }

    net::HttpResponseInfo info;
    info.headers = new net::HttpResponseHeaders(
        net::HttpUtil::AssembleRawHeaders(headers.c_str(), headers.length()));

    ResourceResponseHead response;
    response.headers = info.headers;
    response.headers->GetMimeType(&response.mime_type);

    client->OnReceiveResponse(response, base::nullopt, nullptr);

    mojo::DataPipe data_pipe;

    uint32_t bytes_written = body.size();
    data_pipe.producer_handle->WriteData(body.data(), &bytes_written,
                                         MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
    client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));
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

// ServiceWorkerScriptURLLoaderTest is for testing the handling of requests for
// installing service worker scripts via ServiceWorkerScriptURLLoader.
class ServiceWorkerScriptURLLoaderTest : public testing::Test {
 public:
  ServiceWorkerScriptURLLoaderTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP) {}
  ~ServiceWorkerScriptURLLoaderTest() override = default;

  void SetUp() override {
    helper_ = base::MakeUnique<EmbeddedWorkerTestHelper>(base::FilePath());

    int routing_id = 0;
    int request_id = 10;
    uint32_t options = 0;

    ResourceRequest resource_request;

    loader_factory_getter_ = base::MakeRefCounted<URLLoaderFactoryGetter>();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ServiceWorkerScriptURLLoaderTest::InitializeFactory,
                       base::Unretained(this)));

    InitializeFactory();

    GURL scope("https://www.example.com/");
    GURL script_url("https://example.com/service_worker.js");
    SetUpRegistration(scope, script_url);
    PrepareProviderForServiceWorkerContext(version_.get(), scope);
    provider_host_->running_hosted_version_ = version_;

    loader_ = base::MakeUnique<ServiceWorkerScriptURLLoader>(
        routing_id, request_id, options, resource_request,
        client_.CreateInterfacePtr(), helper_->context()->AsWeakPtr(),
        provider_host_, loader_factory_getter_,
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  // TODO: Copied from appcache_update_job_unittest.cc
  void InitializeFactory() {
    // TODO(nhiroki): Check if we're on the IO thread.
    mojom::URLLoaderFactoryPtr test_loader_factory;
    MockURLLoaderFactory::Create(&test_loader_factory);
    loader_factory_getter_->SetNetworkFactroyForTesting(
        std::move(test_loader_factory));
  }

  // TODO: Copied from service_worker_dispatcher_host_unittest.cc
  void SetUpRegistration(const GURL& scope, const GURL& script_url) {
    registration_ =
        new ServiceWorkerRegistration(ServiceWorkerRegistrationOptions(scope),
                                      1L, helper_->context()->AsWeakPtr());
    version_ = new ServiceWorkerVersion(registration_.get(), script_url, 1L,
                                        helper_->context()->AsWeakPtr());
    std::vector<ServiceWorkerDatabase::ResourceRecord> records;
    records.push_back(
        ServiceWorkerDatabase::ResourceRecord(10, version_->script_url(), 100));
    version_->script_cache_map()->SetResources(records);
    version_->SetMainScriptHttpResponseInfo(
        EmbeddedWorkerTestHelper::CreateHttpResponseInfo());
    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::EXISTS);
    version_->SetStatus(ServiceWorkerVersion::INSTALLING);

    // Make the registration findable via storage functions.
    helper_->context()->storage()->LazyInitialize(base::Bind(&base::DoNothing));
    base::RunLoop().RunUntilIdle();
    bool called = false;
    ServiceWorkerStatusCode status = SERVICE_WORKER_ERROR_MAX_VALUE;
    helper_->context()->storage()->StoreRegistration(
        registration_.get(), version_.get(),
        base::Bind(&SaveStatusCallback, &called, &status));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(called);
    EXPECT_EQ(SERVICE_WORKER_OK, status);
  }

  void PrepareProviderForServiceWorkerContext(ServiceWorkerVersion* version,
                                              const GURL& scope) {
    std::unique_ptr<ServiceWorkerProviderHost> host =
        CreateProviderHostForServiceWorkerContext(
            helper_->mock_render_process_id(),
            true /* is_parent_frame_secure */, version,
            helper_->context()->AsWeakPtr(), &remote_endpoint_);
    provider_host_ = host.get();
    helper_->SimulateAddProcessToPattern(scope,
                                         helper_->mock_render_process_id());
    helper_->context()->AddProviderHost(std::move(host));
  }

 private:
  TestBrowserThreadBundle thread_bundle_;

  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  ServiceWorkerProviderHost* provider_host_;

  TestURLLoaderClient client_;
  std::unique_ptr<ServiceWorkerSCriptURLLoader> loader_;
  scoped_refptr<URLLoaderFactoryGetter> loader_factory_getter_;

  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;
};

}  // namespace blink
